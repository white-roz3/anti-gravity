#include <stdio.h>
#include <string.h>

#include "bridge.h"
#include "net.h"
#include "wipeout/game.h"
#include "wipeout/race.h"
#include "utils.h"

// React shell bridge. The HUD telemetry is pushed C -> JS via EM_ASM (no
// exported symbols needed for this direction). JS -> C commands (P2/P3) use
// EMSCRIPTEN_KEEPALIVE exports + Module.ccall instead. The whole file is a
// no-op on native (only the WASM/React shell consumes it).

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>

void ag_hud_push(ship_t *me, int phase, int paused) {
	int display_lap = max(0, me->lap + 1);

	// On-screen speed readout mirrors hud.c (kph = speed / 9, integer).
	int kph = (int)(me->speed / 9.0f);

	// Thrust bar fraction, same scaling as the C speedo (hud.c: thrust / 65).
	float thrust = me->thrust_mag / 65.0f;
	if (thrust < 0) thrust = 0;
	if (thrust > 1) thrust = 1;

	// g.race_time is only summed at race_end, so derive a live total here:
	// completed laps + the current lap clock.
	float race_time = me->lap_time;
	for (int i = 0; i < me->lap && i < NUM_LAPS; i++) {
		race_time += g.lap_times[me->pilot][i];
	}

	float lap_record = save.highscores[g.race_class][g.circut][g.highscore_tab].lap_record;

	// Note: every comma in the JS below sits inside the onFrame(...) parens, so
	// the preprocessor groups the brace block as a single EM_ASM code argument.
	EM_ASM({
		if (window.AG && window.AG.onFrame) {
			window.AG.onFrame($0, $1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13, $14, $15);
		}
	},
		GAME_SCENE_RACE,        /* $0  scene      */
		g.race_type,            /* $1  raceType   */
		display_lap,            /* $2  lap        */
		NUM_LAPS,               /* $3  laps       */
		me->position_rank,      /* $4  position   */
		NUM_PILOTS,             /* $5  numShips   */
		kph,                    /* $6  speed kph  */
		(double)thrust,         /* $7  thrust 0..1*/
		(double)me->lap_time,   /* $8  lapTime s  */
		(double)race_time,      /* $9  raceTime s */
		(double)lap_record,     /* $10 lapRecord s*/
		me->weapon_type,        /* $11 weapon     */
		g.lives,                /* $12 lives      */
		me->flags,              /* $13 flags      */
		phase,                  /* $14 phase      */
		paused                  /* $15 paused     */
	);
}

// C -> JS scene notify, once per transition (called from game_update's swap).
void ag_on_scene(int scene) {
	const char *name =
		scene == GAME_SCENE_RACE     ? "race" :
		scene == GAME_SCENE_FRONTEND ? "frontend" :
		scene == GAME_SCENE_ONLINE   ? "online" :
		                               "other";
	EM_ASM({
		if (window.AG && window.AG.onScene) { window.AG.onScene(UTF8ToString($0)); }
	}, name);
}

// C -> JS online lobby status (net_state_t + peer count), pushed each frame
// while in the online scene.
void ag_on_online(int state, int peers) {
	EM_ASM({
		if (window.AG && window.AG.onOnline) { window.AG.onOnline($0, $1); }
	}, state, peers);
}

// JS -> C: configure the race and enter it. Mirrors the in-engine menu's launch
// sequence (main_menu.c button_*_select). `pilot` is the ABSOLUTE pilot index.
EMSCRIPTEN_KEEPALIVE void ag_start_race(int race_class, int race_type, int team, int pilot, int circut) {
	// Honor the rapier lock, same as button_race_class_select.
	if (!save.has_rapier_class && race_class == RACE_CLASS_RAPIER) {
		return;
	}
	// Clear the mode flags the menu path would have reset (main_menu_init).
	g.is_attract_mode = false;
	g.is_online = false;

	g.race_class = race_class;
	g.race_type = race_type;
	g.highscore_tab = (race_type == RACE_TYPE_TIME_TRIAL) ? HIGHSCORE_TAB_TIME_TRIAL : HIGHSCORE_TAB_RACE;
	g.team = team;
	g.pilot = pilot;

	if (race_type == RACE_TYPE_CHAMPIONSHIP) {
		g.circut = 0;
		game_reset_championship();
	}
	else {
		g.circut = circut;
	}

	game_set_scene(GAME_SCENE_RACE);
}

// JS -> C: unlock/installed state so the React menu can grey out locked options.
EMSCRIPTEN_KEEPALIVE const char *ag_get_state_json(void) {
	static char buf[512];
	int n = 0;
	n += snprintf(buf + n, sizeof(buf) - n, "{\"hasRapier\":%s,\"hasBonus\":%s,\"installed\":[",
		save.has_rapier_class ? "true" : "false",
		save.has_bonus_circuts ? "true" : "false");
	for (int i = 0; i < NUM_CIRCUTS && n < (int)sizeof(buf) - 8; i++) {
		n += snprintf(buf + n, sizeof(buf) - n, "%s%d", i ? "," : "", g.installed_circuts[i] ? 1 : 0);
	}
	snprintf(buf + n, sizeof(buf) - n,
		"],\"settings\":{\"musicVolume\":%.3f,\"sfxVolume\":%.3f,\"screenShake\":%.3f,\"drawStats\":%d}}",
		(double)save.music_volume, (double)save.sfx_volume, (double)save.screen_shake, (int)save.draw_stats);
	return buf;
}

// C -> JS: push the end-of-race results payload (called from race_end on WASM).
void ag_on_race_over(void) {
	static char buf[2048];
	int n = 0;
	n += snprintf(buf + n, sizeof(buf) - n,
		"{\"raceType\":%d,\"position\":%d,\"numShips\":%d,\"bestLap\":%.4f,\"raceTime\":%.4f,"
		"\"newLapRecord\":%s,\"newRaceRecord\":%s,\"needsName\":%s,\"qualified\":%s,\"laps\":[",
		g.race_type, g.race_position, NUM_PILOTS, (double)g.best_lap, (double)g.race_time,
		g.is_new_lap_record ? "true" : "false", g.is_new_race_record ? "true" : "false",
		g.is_new_race_record ? "true" : "false", (g.race_position <= QUALIFYING_RANK) ? "true" : "false");
	for (int i = 0; i < NUM_LAPS; i++) {
		n += snprintf(buf + n, sizeof(buf) - n, "%s%.4f", i ? "," : "", (double)g.lap_times[g.pilot][i]);
	}
	n += snprintf(buf + n, sizeof(buf) - n, "],\"racePoints\":[");
	for (int i = 0; i < NUM_PILOTS && n < (int)sizeof(buf) - 64; i++) {
		n += snprintf(buf + n, sizeof(buf) - n, "%s{\"name\":\"%s\",\"points\":%d}", i ? "," : "",
			def.pilots[g.race_ranks[i].pilot].name, g.race_ranks[i].points);
	}
	n += snprintf(buf + n, sizeof(buf) - n, "],\"standings\":[");
	for (int i = 0; i < NUM_PILOTS && n < (int)sizeof(buf) - 64; i++) {
		n += snprintf(buf + n, sizeof(buf) - n, "%s{\"name\":\"%s\",\"points\":%d}", i ? "," : "",
			def.pilots[g.championship_ranks[i].pilot].name, g.championship_ranks[i].points);
	}
	snprintf(buf + n, sizeof(buf) - n, "]}");
	EM_ASM({ if (window.AG && window.AG.onRaceOver) { window.AG.onRaceOver(UTF8ToString($0)); } }, buf);
}

// JS -> C race controls (mirror the in-engine pause/results buttons).
EMSCRIPTEN_KEEPALIVE void ag_pause(void)   { race_pause(); }
EMSCRIPTEN_KEEPALIVE void ag_unpause(void) { race_unpause(); }
EMSCRIPTEN_KEEPALIVE void ag_restart(void) { race_restart(); }
EMSCRIPTEN_KEEPALIVE void ag_quit_to_menu(void) { game_set_scene(GAME_SCENE_MAIN_MENU); }

EMSCRIPTEN_KEEPALIVE void ag_results_continue(void) {
	if (g.race_type == RACE_TYPE_CHAMPIONSHIP) {
		race_next();
	}
	else {
		game_set_scene(GAME_SCENE_MAIN_MENU);
	}
}

// JS -> C: insert a hall-of-fame name for a new race record (replicates the
// insert in ingame_menus.c), then advance championship.
EMSCRIPTEN_KEEPALIVE void ag_submit_name(const char *name) {
	highscores_t *hs = &save.highscores[g.race_class][g.circut][g.highscore_tab];
	highscores_entry_t e;
	strncpy(e.name, (name && name[0]) ? name : "AAA", 3);
	e.name[3] = '\0';
	e.time = g.race_time;
	for (int i = 0; i < NUM_HIGHSCORES; i++) {
		if (e.time < hs->entries[i].time) {
			for (int j = NUM_HIGHSCORES - 2; j >= i; j--) {
				hs->entries[j + 1] = hs->entries[j];
			}
			hs->entries[i] = e;
			break;
		}
	}
	strncpy(save.highscores_name, e.name, 4);
	save.is_dirty = true;
	if (g.race_type == RACE_TYPE_CHAMPIONSHIP) {
		race_next();
	}
}

// JS -> C: write a save setting + flag dirty (game_update persists it to IDBFS).
EMSCRIPTEN_KEEPALIVE void ag_set_option(int key, double value) {
	switch (key) {
		case 0: save.music_volume = (float)value; break; // AG_OPT.MUSIC_VOLUME
		case 1: save.sfx_volume = (float)value; break;   // AG_OPT.SFX_VOLUME
		case 2: save.screen_shake = (float)value; break; // AG_OPT.SCREEN_SHAKE
		case 3: save.draw_stats = (draw_stats_t)(int)value; break; // AG_OPT.DRAW_STATS
		default: return;
	}
	save.is_dirty = true;
}

// JS -> C: enter the online lobby (connects to the WS server) / cancel back out.
EMSCRIPTEN_KEEPALIVE void ag_online_enter(void) {
	game_set_scene(GAME_SCENE_ONLINE);
}
EMSCRIPTEN_KEEPALIVE void ag_online_cancel(void) {
	net_disconnect();
	game_set_scene(GAME_SCENE_FRONTEND);
}

#else
void ag_hud_push(ship_t *me, int phase, int paused) { (void)me; (void)phase; (void)paused; }
void ag_on_scene(int scene) { (void)scene; }
void ag_on_online(int state, int peers) { (void)state; (void)peers; }
#endif
