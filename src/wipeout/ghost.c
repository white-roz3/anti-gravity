#include <string.h>
#include "../render.h"
#include "../types.h"
#include "object.h"
#include "ship.h"
#include "ship_remote.h"
#include "game.h"
#include "ghost.h"

#define GHOST_MAX_FRAMES (60 * 120) // 2 minutes at the fixed sim rate

// Recording of the run in progress.
static ship_frame_t rec_frames[GHOST_MAX_FRAMES];
static int rec_len;

// The ghost being replayed (a copy of the previous completed recording).
static ship_frame_t play_frames[GHOST_MAX_FRAMES];
static int play_len;
static int play_index;
static bool play_active;

static ship_t g_ghost;

// Buffer-backed remote source. Record and playback both run at the fixed sim
// rate, so playback steps one frame per sim step (no interpolation needed);
// the source just returns the current frame.
typedef struct {
	ship_frame_t *frames;
	int len;
	int *index;
} ghost_source_ctx_t;
static ghost_source_ctx_t g_ghost_ctx;

static bool ghost_sample(void *ctx_, double sim_time, ship_frame_t *a, ship_frame_t *b, float *t) {
	(void)sim_time;
	ghost_source_ctx_t *ctx = ctx_;
	if (ctx->len <= 0) {
		return false;
	}
	int i = *ctx->index;
	if (i >= ctx->len) {
		i = ctx->len - 1;
	}
	*a = ctx->frames[i];
	*b = ctx->frames[i];
	*t = 0;
	return true;
}

void ghost_reset(void) {
	rec_len = 0;
	play_len = 0;
	play_index = 0;
	play_active = false;
}

void ghost_record_tick(ship_t *player) {
	if (rec_len >= GHOST_MAX_FRAMES) {
		return;
	}
	rec_frames[rec_len].position = player->position;
	rec_frames[rec_len].angle = player->angle;
	rec_frames[rec_len].section_num = player->section_num;
	rec_frames[rec_len].lap = player->lap;
	rec_len++;
}

void ghost_begin_race(void) {
	if (g.race_type != RACE_TYPE_TIME_TRIAL) {
		play_active = false;
		rec_len = 0;
		return;
	}

	// Promote the previous run's recording to the playback ghost.
	if (rec_len > 0) {
		memcpy(play_frames, rec_frames, sizeof(ship_frame_t) * rec_len);
		play_len = rec_len;
	}
	// Start recording the new run.
	rec_len = 0;

	play_index = 0;
	play_active = (play_len > 0);
	if (!play_active) {
		return;
	}

	// Stand up the ghost as a translucent puppet of the player's ship: share
	// the model (read-only for drawing), drive position/angle from the buffer.
	ship_t *player = &g.ships[g.pilot];
	memset(&g_ghost, 0, sizeof(g_ghost));
	g_ghost.model = player->model;
	g_ghost.pilot = player->pilot;
	g_ghost.section = player->section;
	g_ghost.flags = SHIP_VISIBLE;
	g_ghost.mat = mat4_identity();

	g_ghost_ctx.frames = play_frames;
	g_ghost_ctx.len = play_len;
	g_ghost_ctx.index = &play_index;
	remote_source_t src = { .sample = ghost_sample, .ctx = &g_ghost_ctx };
	ship_set_remote(&g_ghost, src);
}

void ghost_update(void) {
	if (!play_active) {
		return;
	}
	ship_remote_update(&g_ghost);
	if (play_index < play_len) {
		play_index++;
	}
}

// The ghost shares the player's ship model, so we can't permanently recolor it.
// Instead, scale every primitive's alpha down (dim=true) just for the ghost's
// draw, then restore (dim=false). render_push_tris bakes vertex colors at push
// time, so the player's already-submitted geometry (drawn at full alpha before
// the ghost) is unaffected.
#define GHOST_ALPHA 150 // out of 255 — clearly visible but see-through

static uint8_t ghost_saved_alpha[8192];

static void ghost_model_set_alpha(Object *m, bool dim) {
	Prm prm = { .primitive = m->primitives };
	int s = 0;
	for (int i = 0; i < m->primitives_len; i++) {
		switch (prm.primitive->type) {
		case PRM_TYPE_F3:
			if (dim) { ghost_saved_alpha[s++] = prm.f3->color.a; prm.f3->color.a = GHOST_ALPHA; }
			else { prm.f3->color.a = ghost_saved_alpha[s++]; }
			prm.f3 += 1; break;
		case PRM_TYPE_FT3:
			if (dim) { ghost_saved_alpha[s++] = prm.ft3->color.a; prm.ft3->color.a = GHOST_ALPHA; }
			else { prm.ft3->color.a = ghost_saved_alpha[s++]; }
			prm.ft3 += 1; break;
		case PRM_TYPE_F4:
			if (dim) { ghost_saved_alpha[s++] = prm.f4->color.a; prm.f4->color.a = GHOST_ALPHA; }
			else { prm.f4->color.a = ghost_saved_alpha[s++]; }
			prm.f4 += 1; break;
		case PRM_TYPE_FT4:
			if (dim) { ghost_saved_alpha[s++] = prm.ft4->color.a; prm.ft4->color.a = GHOST_ALPHA; }
			else { prm.ft4->color.a = ghost_saved_alpha[s++]; }
			prm.ft4 += 1; break;
		case PRM_TYPE_G3:
			for (int j = 0; j < 3; j++) { if (dim) { ghost_saved_alpha[s++] = prm.g3->color[j].a; prm.g3->color[j].a = GHOST_ALPHA; } else { prm.g3->color[j].a = ghost_saved_alpha[s++]; } }
			prm.g3 += 1; break;
		case PRM_TYPE_GT3:
			for (int j = 0; j < 3; j++) { if (dim) { ghost_saved_alpha[s++] = prm.gt3->color[j].a; prm.gt3->color[j].a = GHOST_ALPHA; } else { prm.gt3->color[j].a = ghost_saved_alpha[s++]; } }
			prm.gt3 += 1; break;
		case PRM_TYPE_G4:
			for (int j = 0; j < 4; j++) { if (dim) { ghost_saved_alpha[s++] = prm.g4->color[j].a; prm.g4->color[j].a = GHOST_ALPHA; } else { prm.g4->color[j].a = ghost_saved_alpha[s++]; } }
			prm.g4 += 1; break;
		case PRM_TYPE_GT4:
			for (int j = 0; j < 4; j++) { if (dim) { ghost_saved_alpha[s++] = prm.gt4->color[j].a; prm.gt4->color[j].a = GHOST_ALPHA; } else { prm.gt4->color[j].a = ghost_saved_alpha[s++]; } }
			prm.gt4 += 1; break;
		default:
			prm.f3 += 1; break;
		}
	}
}

void ghost_draw(void) {
	if (!play_active) {
		return;
	}
	// Translucent (reduced per-vertex alpha under normal blend), depth-write off
	// so it blends cleanly over the scene without occluding later geometry.
	render_set_depth_write(false);
	ghost_model_set_alpha(g_ghost.model, true);
	ship_draw(&g_ghost);
	ghost_model_set_alpha(g_ghost.model, false);
	render_set_depth_write(true);
}

bool ghost_is_playing(void) {
	return play_active;
}
