#ifndef NET_H
#define NET_H

#include <stdint.h>
#include <stdbool.h>
#include "wipeout/ship.h" // remote_source_t, ship_frame_t

// WebSocket transport for online multiplayer (browser-only; native = no-op stub).

typedef enum {
	NET_CLOSED,
	NET_CONNECTING,
	NET_OPEN,
	NET_UNSUPPORTED,
} net_state_t;

// Ship-state packet a client sends each network tick. The server prepends a
// 1-byte sender slot before relaying. Sent raw (little-endian host layout).
typedef struct __attribute__((__packed__)) {
	uint16_t seq;
	float pos[3];
	float vel[3];
	float ang[3];
	int16_t section;
	int16_t lap;
	uint16_t flags;
} net_state_packet_t; // 44 bytes, no padding

// Race-start parameters the server sends once a room is ready.
typedef struct {
	uint32_t seed;
	int my_slot;
	int track;
	int race_class;
	int active_slots[8];
	int active_count;
} net_start_t;

void net_connect(const char *url);
void net_disconnect(void);
void net_send_text(const char *s);
void net_send(const void *data, int len);
void net_poll(void);

net_state_t net_state(void);
bool     net_is_open(void);
int      net_peers(void);
uint32_t net_rx_count(void);

// --- multiplayer race ---
bool net_start_pending(void);            // a START arrived (cleared by consume)
void net_consume_start(net_start_t *out);
void net_send_state(const void *packet, int len);  // broadcast the local ship's state
bool net_slot_active(int slot);          // a remote peer is currently feeding this slot
remote_source_t net_remote_source(int slot); // pass to ship_set_remote()

#endif
