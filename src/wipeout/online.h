#ifndef ONLINE_H
#define ONLINE_H

// Online-race lobby scene: connect to the relay, wait for a second player, then
// the server's START hands off into a normal race with the other player's ship
// driven as a network puppet.
void online_init(void);
void online_update(void);

#endif
