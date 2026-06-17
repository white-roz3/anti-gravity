#ifndef NET_H
#define NET_H

#include <stdint.h>
#include <stdbool.h>

// Minimal WebSocket transport for online multiplayer (browser-only). On native
// builds every function is a no-op stub and net_state() returns NET_UNSUPPORTED.
//
// MP Step 0 uses this to prove connectivity (connect + relay). Later steps send
// binary race-state packets over the same socket. Callbacks run on the browser
// main thread (no threads), so the counters below are safe to read from the game
// loop without locking.

typedef enum {
	NET_CLOSED,
	NET_CONNECTING,
	NET_OPEN,
	NET_UNSUPPORTED,
} net_state_t;

void     net_connect(const char *url);
void     net_disconnect(void);
void     net_send_text(const char *s);
void     net_send(const void *data, int len);
void     net_poll(void);            // call once per frame (reserved for future queue draining)

net_state_t net_state(void);
bool     net_is_open(void);
int      net_peers(void);           // peer count reported by the relay
uint32_t net_rx_count(void);        // number of relayed messages received

#endif
