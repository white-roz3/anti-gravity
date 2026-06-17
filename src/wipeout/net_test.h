#ifndef NET_TEST_H
#define NET_TEST_H

// MP Step 0 scene: connect to the relay server and show connection status
// (WS state, peer count, received-message count) so connectivity can be proven
// with two browser tabs before any race sync is built.
void net_test_init(void);
void net_test_update(void);

#endif
