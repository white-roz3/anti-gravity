#ifndef GHOST_H
#define GHOST_H

#include "ship.h"

// Local time-trial ghost: records the player's run at the fixed sim rate and
// replays the previous run as a translucent standalone puppet (driven through
// the A3 remote-ship hook). No network — pure local replay. Persistence and
// downloaded leaderboard ghosts build on top of this later.

void ghost_reset(void);                 // clear all ghost state
void ghost_begin_race(void);            // race_start(): promote last run → playback, start a fresh recording (TT only)
void ghost_record_tick(ship_t *player); // each fixed sim step (TT): append the player's frame
void ghost_update(void);                // each fixed sim step (TT): advance the playback puppet
void ghost_draw(void);                  // 3D pass after ships_draw(): draw the translucent ghost
bool ghost_is_playing(void);

#endif
