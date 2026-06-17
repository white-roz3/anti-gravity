#ifndef SHIP_REMOTE_H
#define SHIP_REMOTE_H

#include "ship.h"

// Per-tick update for a remote/puppet ship. Instead of running physics it pulls
// an interpolated transform from the ship's attached remote_source and applies
// it. Assigned as the ship's update_func, so the surrounding ship_update() still
// rebuilds the transform matrix and does lap/section bookkeeping afterwards.
//
// This single hook is shared by ghost playback (a buffer-backed source) and, in
// the future, live multiplayer (a network-backed source).
void ship_remote_update(ship_t *self);

// Switch a ship into remote/puppet mode, driven by `source`.
void ship_set_remote(ship_t *self, remote_source_t source);

// Per-tick update for a remote ship inside a multiplayer race: applies the
// networked transform (via ship_remote_update) and recomputes total_section_num
// so standings work — but runs NO physics / pickup / lap-crossing. Use this in
// the ships_update loop for REMOTE ships instead of ship_update().
void ship_remote_tick(ship_t *self);

#endif
