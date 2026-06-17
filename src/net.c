#include "net.h"
#include <string.h>
#include <stdlib.h>

#ifdef __EMSCRIPTEN__

#include <emscripten/websocket.h>

static EMSCRIPTEN_WEBSOCKET_T g_sock = 0;
static net_state_t g_state = NET_CLOSED;
static int g_peers = 0;
static uint32_t g_rx = 0;

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
	// The relay sends "peers=<N>" presence updates as text; everything else is a
	// relayed peer message (counted as RX).
	if (e->isText && e->numBytes >= 6 && memcmp(e->data, "peers=", 6) == 0) {
		g_peers = atoi((const char *)e->data + 6);
	}
	else {
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
	g_peers = 0;
	g_rx = 0;
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

void net_poll(void) {
	// Callbacks fire on the browser main thread; nothing to drain yet.
}

net_state_t net_state(void) { return g_state; }
bool net_is_open(void) { return g_state == NET_OPEN; }
int net_peers(void) { return g_peers; }
uint32_t net_rx_count(void) { return g_rx; }

#else // ---- native: no browser WebSocket; stubs so the build links ----

void net_connect(const char *url) { (void)url; }
void net_disconnect(void) {}
void net_send_text(const char *s) { (void)s; }
void net_send(const void *data, int len) { (void)data; (void)len; }
void net_poll(void) {}
net_state_t net_state(void) { return NET_UNSUPPORTED; }
bool net_is_open(void) { return false; }
int net_peers(void) { return 0; }
uint32_t net_rx_count(void) { return 0; }

#endif
