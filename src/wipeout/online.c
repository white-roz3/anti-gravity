#include "../net.h"
#include "../system.h"
#include "../input.h"
#include "../render.h"

#include "game.h"
#include "ui.h"
#include "online.h"
#include "../bridge.h"

#ifndef NET_SERVER_URL
#define NET_SERVER_URL "wss://ag-mp-server-production.up.railway.app"
#endif

static double last_send;

void online_init(void) {
	net_connect(NET_SERVER_URL);
	last_send = system_time();
}

void online_update(void) {
	net_poll();

	// Keep the socket warm while waiting for the second player.
	double now = system_time();
	if (net_is_open() && now - last_send >= 1.0) {
		net_send_text("hi");
		last_send = now;
	}

	// Server says go: lock in seed/slot/track/class and hand off to the race.
	if (net_start_pending()) {
		net_start_t st;
		net_consume_start(&st);
		g.is_online = true;
		g.online_seed = st.seed;
		g.race_type = RACE_TYPE_SINGLE;
		g.race_class = st.race_class;
		g.circut = st.track;
		g.pilot = st.my_slot;
		for (int i = 0; i < NUM_PILOTS; i++) {
			g.online_remote[i] = false;
		}
		for (int i = 0; i < st.active_count; i++) {
			int s = st.active_slots[i];
			if (s >= 0 && s < NUM_PILOTS && s != st.my_slot) {
				g.online_remote[s] = true;
			}
		}
		game_set_scene(GAME_SCENE_RACE);
		return;
	}

#ifndef __EMSCRIPTEN__
	// On WASM the React lobby owns input (cancel via ag_online_cancel).
	if (input_pressed(A_MENU_QUIT) || input_pressed(A_MENU_BACK)) {
		net_disconnect();
		game_set_scene(GAME_SCENE_MAIN_MENU);
		return;
	}
#endif

	// --- draw / status ---
	render_set_view_2d();
	render_push_2d(vec2i(0, 0), render_size(), rgba(4, 8, 16, 255), RENDER_NO_TEXTURE);

#ifdef __EMSCRIPTEN__
	// React draws the lobby; push the net status (net_state_t + peer count).
	ag_on_online(net_state(), net_peers());
#else
	// NOTE: the bitmap font only has A-Z, 0-9 and space — no punctuation/lowercase
	// (ui_draw_text would read an out-of-bounds glyph), so keep these uppercase.
	const char *status =
		net_is_open()                    ? "WAITING FOR PLAYER" :
		net_state() == NET_CONNECTING    ? "CONNECTING" :
		net_state() == NET_UNSUPPORTED   ? "WEBSOCKET UNSUPPORTED" :
		                                   "DISCONNECTED";

	ui_draw_text_centered("ONLINE RACE", ui_scaled_pos(UI_POS_TOP | UI_POS_CENTER, vec2i(0, 40)), UI_SIZE_16, UI_COLOR_ACCENT);
	ui_draw_text_centered(status, ui_scaled_pos(UI_POS_MIDDLE | UI_POS_CENTER, vec2i(0, -12)), UI_SIZE_8, UI_COLOR_DEFAULT);

	int x = -40, y = 8;
	ui_draw_text("PLAYERS", ui_scaled_pos(UI_POS_MIDDLE | UI_POS_CENTER, vec2i(x, y)), UI_SIZE_8, UI_COLOR_ACCENT);
	ui_draw_number(net_peers(), ui_scaled_pos(UI_POS_MIDDLE | UI_POS_CENTER, vec2i(x + 70, y)), UI_SIZE_8, UI_COLOR_DEFAULT);

	ui_draw_text_centered("PRESS ESC TO CANCEL", ui_scaled_pos(UI_POS_BOTTOM | UI_POS_CENTER, vec2i(0, -36)), UI_SIZE_8, UI_COLOR_DEFAULT);
#endif
}
