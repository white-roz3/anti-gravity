#include "net.h"
#include "system.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#define NET_MAX_SLOTS 8
#define NET_BUF 12              // per-slot jitter-buffer depth
#define NET_INTERP_DELAY 0.12  // render remotes ~120 ms in the past to absorb jitter

// One received frame for a remote slot, tagged with local arrival time.
typedef struct {
	double recv_time;
	float pos[3];
	float vel[3];
	float ang[3];
	int16_t section;
	int16_t lap;
} net_frame_t;

typedef struct {
	net_frame_t frames[NET_BUF];
	int head;   // next write index
	int count;
	bool active;
} net_slot_t;

#ifdef __EMSCRIPTEN__

#include <emscripten/websocket.h>

static EMSCRIPTEN_WEBSOCKET_T g_sock = 0;
static net_state_t g_state = NET_CLOSED;
static int g_peers = 0;
static uint32_t g_rx = 0;

static net_slot_t g_slots[NET_MAX_SLOTS];
static net_start_t g_start;
static bool g_start_pending;

static void clear_slots(void) {
	memset(g_slots, 0, sizeof(g_slots));
}

static void push_state(int slot, const net_state_packet_t *pkt) {
	if (slot < 0 || slot >= NET_MAX_SLOTS) {
		return;
	}
	net_slot_t *s = &g_slots[slot];
	net_frame_t *f = &s->frames[s->head];
	f->recv_time = system_time();
	memcpy(f->pos, pkt->pos, sizeof(f->pos));
	memcpy(f->vel, pkt->vel, sizeof(f->vel));
	memcpy(f->ang, pkt->ang, sizeof(f->ang));
	f->section = pkt->section;
	f->lap = pkt->lap;
	s->head = (s->head + 1) % NET_BUF;
	if (s->count < NET_BUF) {
		s->count++;
	}
	s->active = true;
}

static void parse_start(const char *msg) {
	unsigned seed = 0;
	int slot = 0, track = 0, cls = 0;
	char csv[80] = {0};
	if (sscanf(msg, "start %u %d %d %d %79s", &seed, &slot, &track, &cls, csv) < 4) {
		return;
	}
	g_start.seed = seed;
	g_start.my_slot = slot;
	g_start.track = track;
	g_start.race_class = cls;
	g_start.active_count = 0;
	const char *p = csv;
	while (*p && g_start.active_count < 8) {
		g_start.active_slots[g_start.active_count++] = atoi(p);
		while (*p && *p != ',') p++;
		if (*p == ',') p++;
	}
	g_start_pending = true;
}

static EM_BOOL on_open(int t, const EmscriptenWebSocketOpenEvent *e, void *u) {
	(void)t; (void)e; (void)u;
	g_state = NET_OPEN;
	return EM_TRUE;
}
static EM_BOOL on_error(int t, const EmscriptenWebSocketErrorEvent *e, void *u) {
	(void)t; (void)e; (void)u;
	g_state = NET_CLOSED;
	return EM_TRUE;
}
static EM_BOOL on_close(int t, const EmscriptenWebSocketCloseEvent *e, void *u) {
	(void)t; (void)e; (void)u;
	g_state = NET_CLOSED;
	return EM_TRUE;
}
static EM_BOOL on_message(int t, const EmscriptenWebSocketMessageEvent *e, void *u) {
	(void)t; (void)u;
	if (e->isText) {
		char msg[160];
		int n = e->numBytes < 159 ? (int)e->numBytes : 159;
		memcpy(msg, e->data, n);
		msg[n] = 0;
		if (strncmp(msg, "peers=", 6) == 0) {
			g_peers = atoi(msg + 6);
		}
		else if (strncmp(msg, "start ", 6) == 0) {
			parse_start(msg);
		}
		else if (strncmp(msg, "leave ", 6) == 0) {
			int slot = atoi(msg + 6);
			if (slot >= 0 && slot < NET_MAX_SLOTS) {
				g_slots[slot].active = false;
			}
		}
		else {
			g_rx++;
		}
	}
	else if (e->numBytes >= 1 + (int)sizeof(net_state_packet_t)) {
		// [slot u8][net_state_packet_t]
		const uint8_t *d = (const uint8_t *)e->data;
		net_state_packet_t pkt;
		memcpy(&pkt, d + 1, sizeof(pkt));
		push_state(d[0], &pkt);
		g_rx++;
	}
	return EM_TRUE;
}

void net_connect(const char *url) {
	if (!emscripten_websocket_is_supported()) {
		g_state = NET_UNSUPPORTED;
		return;
	}
	if (g_sock) {
		net_disconnect();
	}
	clear_slots();
	g_start_pending = false;
	g_peers = 0;
	g_rx = 0;

	EmscriptenWebSocketCreateAttributes attrs;
	emscripten_websocket_init_create_attributes(&attrs);
	attrs.url = url;
	attrs.protocols = NULL;

	g_sock = emscripten_websocket_new(&attrs);
	if (g_sock <= 0) {
		g_sock = 0;
		g_state = NET_CLOSED;
		return;
	}
	g_state = NET_CONNECTING;
	emscripten_websocket_set_onopen_callback(g_sock, NULL, on_open);
	emscripten_websocket_set_onerror_callback(g_sock, NULL, on_error);
	emscripten_websocket_set_onclose_callback(g_sock, NULL, on_close);
	emscripten_websocket_set_onmessage_callback(g_sock, NULL, on_message);
}

void net_disconnect(void) {
	if (g_sock) {
		emscripten_websocket_close(g_sock, 1000, "bye");
		emscripten_websocket_delete(g_sock);
		g_sock = 0;
	}
	g_state = NET_CLOSED;
	g_peers = 0;
	clear_slots();
}

void net_send_text(const char *s) {
	if (g_sock && g_state == NET_OPEN) {
		emscripten_websocket_send_utf8_text(g_sock, s);
	}
}

void net_send(const void *data, int len) {
	if (g_sock && g_state == NET_OPEN) {
		emscripten_websocket_send_binary(g_sock, (void *)data, len);
	}
}

void net_send_state(const void *packet, int len) {
	net_send(packet, len);
}

void net_poll(void) {
	// Emscripten WS callbacks run on the main thread; nothing to drain.
}

net_state_t net_state(void) { return g_state; }
bool net_is_open(void) { return g_state == NET_OPEN; }
int net_peers(void) { return g_peers; }
uint32_t net_rx_count(void) { return g_rx; }

bool net_start_pending(void) { return g_start_pending; }
void net_consume_start(net_start_t *out) { *out = g_start; g_start_pending = false; }
bool net_slot_active(int slot) {
	return slot >= 0 && slot < NET_MAX_SLOTS && g_slots[slot].active && g_slots[slot].count > 0;
}

static void frame_to_ship(const net_frame_t *f, ship_frame_t *out) {
	out->position = vec3(f->pos[0], f->pos[1], f->pos[2]);
	out->angle = vec3(f->ang[0], f->ang[1], f->ang[2]);
	out->section_num = f->section;
	out->lap = f->lap;
}

// remote_source_t.sample: interpolate the slot's jitter buffer at (now - delay).
static bool net_sample(void *ctx, double sim_time, ship_frame_t *a, ship_frame_t *b, float *t) {
	int slot = (int)(intptr_t)ctx;
	if (slot < 0 || slot >= NET_MAX_SLOTS) {
		return false;
	}
	net_slot_t *s = &g_slots[slot];
	if (!s->active || s->count == 0) {
		return false;
	}

	double target = sim_time - NET_INTERP_DELAY;
	int oldest = (s->head - s->count + NET_BUF) % NET_BUF;

	int ia = 0;
	for (int i = 0; i < s->count; i++) {
		int idx = (oldest + i) % NET_BUF;
		if (s->frames[idx].recv_time <= target) {
			ia = i;
		}
		else {
			break;
		}
	}

	int idx_a = (oldest + ia) % NET_BUF;
	if (ia >= s->count - 1) {
		// at/after newest — hold (no extrapolation in MVP)
		frame_to_ship(&s->frames[idx_a], a);
		*b = *a;
		*t = 0;
		return true;
	}

	int idx_b = (oldest + ia + 1) % NET_BUF;
	net_frame_t *fa = &s->frames[idx_a];
	net_frame_t *fb = &s->frames[idx_b];
	frame_to_ship(fa, a);
	frame_to_ship(fb, b);
	double span = fb->recv_time - fa->recv_time;
	float tt = (span > 1e-6) ? (float)((target - fa->recv_time) / span) : 0.0f;
	*t = tt < 0 ? 0 : (tt > 1 ? 1 : tt);
	return true;
}

remote_source_t net_remote_source(int slot) {
	remote_source_t src = { .sample = net_sample, .ctx = (void *)(intptr_t)slot };
	return src;
}

#else // ---- native: no browser WebSocket; stubs so the build links ----

void net_connect(const char *url) { (void)url; }
void net_disconnect(void) {}
void net_send_text(const char *s) { (void)s; }
void net_send(const void *data, int len) { (void)data; (void)len; }
void net_send_state(const void *packet, int len) { (void)packet; (void)len; }
void net_poll(void) {}
net_state_t net_state(void) { return NET_UNSUPPORTED; }
bool net_is_open(void) { return false; }
int net_peers(void) { return 0; }
uint32_t net_rx_count(void) { return 0; }
bool net_start_pending(void) { return false; }
void net_consume_start(net_start_t *out) { memset(out, 0, sizeof(*out)); }
bool net_slot_active(int slot) { (void)slot; return false; }
remote_source_t net_remote_source(int slot) { (void)slot; remote_source_t s = {0}; return s; }

#endif
