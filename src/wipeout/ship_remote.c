#include <math.h>
#include "../types.h"
#include "../system.h"
#include "ship.h"
#include "ship_remote.h"

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
