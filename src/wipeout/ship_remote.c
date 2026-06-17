#include <math.h>
#include "../types.h"
#include "../system.h"
#include "ship.h"
#include "ship_remote.h"
#include "game.h"
#include "track.h"

// Shortest-arc interpolation between two wrapped angles (radians).
static float lerp_angle(float a, float b, float t) {
	float d = b - a;
	while (d >  M_PI) d -= 2.0 * M_PI;
	while (d < -M_PI) d += 2.0 * M_PI;
	return a + d * t;
}

void ship_remote_update(ship_t *self) {
	if (!self->remote_source.sample) {
		return;
	}

	ship_frame_t a, b;
	float t = 0;
	if (!self->remote_source.sample(self->remote_source.ctx, system_time(), &a, &b, &t)) {
		return;
	}

	// Interpolated transform — no physics.
	self->position = vec3_add(a.position, vec3_mulf(vec3_sub(b.position, a.position), t));
	self->angle = vec3(
		lerp_angle(a.angle.x, b.angle.x, t),
		lerp_angle(a.angle.y, b.angle.y, t),
		lerp_angle(a.angle.z, b.angle.z, t)
	);
	self->section_num = b.section_num;
	self->lap = b.lap;

	// Self-contained: rebuild the transform matrix and re-derive the track
	// section (for the shadow), so a puppet works whether driven standalone
	// (ghost) or from within ship_update() (a networked slot).
	float dist;
	self->section = track_nearest_section(self->position, vec3(1, 0.25, 1), self->section, &dist);
	mat4_set_translation(&self->mat, self->position);
	mat4_set_yaw_pitch_roll(&self->mat, self->angle);
}

void ship_set_remote(ship_t *self, remote_source_t source) {
	self->control = SHIP_CONTROL_REMOTE;
	self->remote_source = source;
	self->update_func = ship_remote_update;
}

void ship_remote_tick(ship_t *self) {
	self->prev_section = self->section;
	self->prev_section_num = self->section_num;

	ship_remote_update(self); // networked transform; sets section_num + lap; rebuilds mat

	// Recompute total_section_num for standings (mirrors ship_update's tail) —
	// no physics, pickups, or lap-crossing detection for a puppet.
	int start_line_pos = def.circuts[g.circut].settings[g.race_class].start_line_pos;
	int section_num_from_line = self->section_num - (start_line_pos + 1);
	if (section_num_from_line < 0) {
		section_num_from_line += g.track.section_count;
	}
	self->total_section_num = self->lap * g.track.section_count + section_num_from_line;
}
