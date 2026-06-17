#include "../render.h"

#include "game.h"
#include "frontend.h"

// The React shell owns the UI on WASM; this scene only keeps the engine's frame
// loop alive behind the overlay. (Native builds never enter this scene.)

void frontend_init(void) {
	// Intentionally empty — no assets, no attract setup.
}

void frontend_update(void) {
	// Clear to a solid dark color so the canvas under the React menu isn't stale
	// garbage. No input handling — scene changes come from JS (ag_start_race).
	render_set_view_2d();
	render_push_2d(vec2i(0, 0), render_size(), rgba(4, 8, 16, 255), RENDER_NO_TEXTURE);
}
