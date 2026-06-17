#ifndef FRONTEND_H
#define FRONTEND_H

// Dormant WASM boot scene. Clears the framebuffer each frame and does nothing
// else — the React shell draws all menus over the canvas and enters gameplay by
// calling the exported ag_start_race(). No attract mode, no input, no assets.
void frontend_init(void);
void frontend_update(void);

#endif
