#ifndef AG_BRIDGE_H
#define AG_BRIDGE_H

#include "wipeout/ship.h"

// React shell bridge (C -> JS). On the WASM build this pushes one telemetry
// frame per displayed frame to window.AG.onFrame so the React HUD overlay can
// render. No-op on native builds.
//   phase: 0 = countdown/intro, 1 = racing.  paused: 0/1.
void ag_hud_push(ship_t *me, int phase, int paused);

// C -> JS scene-change notify (calls window.AG.onScene("frontend"|"race"|...)).
// No-op on native.
void ag_on_scene(int scene);

#ifdef __EMSCRIPTEN__
// C -> JS: push end-of-race results (window.AG.onRaceOver). Called from race_end.
void ag_on_race_over(void);

// JS -> C commands (called from the React shell via Module.ccall). WASM-only.
void ag_start_race(int race_class, int race_type, int team, int pilot, int circut);
const char *ag_get_state_json(void);
void ag_pause(void);
void ag_unpause(void);
void ag_restart(void);
void ag_quit_to_menu(void);
void ag_results_continue(void);
void ag_submit_name(const char *name);
void ag_set_option(int key, double value);
#endif

#endif
