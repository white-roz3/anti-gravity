#include "../net.h"
#include "../system.h"
#include "../input.h"
#include "../render.h"

#include "game.h"
#include "ui.h"
#include "net_test.h"

#ifndef NET_SERVER_URL
#define NET_SERVER_URL "wss://ag-mp-server-production.up.railway.app"
#endif

static double last_send;

void net_test_init(void) {
	net_connect(NET_SERVER_URL);
	last_send = system_time();
}

void net_test_update(void) {
	net_poll();

	// Heartbeat ~1/s so each peer's RX counter reflects the others' presence.
	double now = system_time();
	if (net_is_open() && now - last_send >= 1.0) {
		net_send_text("hi");
		last_send = now;
	}

	if (input_pressed(A_MENU_QUIT) || input_pressed(A_MENU_BACK)) {
		net_disconnect();
		game_set_scene(GAME_SCENE_MAIN_MENU);
		return;
	}

	// --- draw status ---
	render_set_view_2d();
	render_push_2d(vec2i(0, 0), render_size(), rgba(4, 8, 16, 255), RENDER_NO_TEXTURE);

	const char *status = "?";
	switch (net_state()) {
		case NET_CONNECTING:  status = "CONNECTING"; break;
		case NET_OPEN:        status = "OPEN"; break;
		case NET_CLOSED:      status = "CLOSED"; break;
		case NET_UNSUPPORTED: status = "UNSUPPORTED"; break;
	}

	ui_draw_text_centered("NETWORK TEST", ui_scaled_pos(UI_POS_TOP | UI_POS_CENTER, vec2i(0, 40)), UI_SIZE_16, UI_COLOR_ACCENT);

	int x = -70, y = -24;
	ui_draw_text("WS",    ui_scaled_pos(UI_POS_MIDDLE | UI_POS_CENTER, vec2i(x, y)),      UI_SIZE_8, UI_COLOR_ACCENT);
	ui_draw_text(status,  ui_scaled_pos(UI_POS_MIDDLE | UI_POS_CENTER, vec2i(x + 60, y)), UI_SIZE_8, UI_COLOR_DEFAULT);
	y += 18;
	ui_draw_text("PEERS", ui_scaled_pos(UI_POS_MIDDLE | UI_POS_CENTER, vec2i(x, y)),      UI_SIZE_8, UI_COLOR_ACCENT);
	ui_draw_number(net_peers(), ui_scaled_pos(UI_POS_MIDDLE | UI_POS_CENTER, vec2i(x + 60, y)), UI_SIZE_8, UI_COLOR_DEFAULT);
	y += 18;
	ui_draw_text("RX",    ui_scaled_pos(UI_POS_MIDDLE | UI_POS_CENTER, vec2i(x, y)),      UI_SIZE_8, UI_COLOR_ACCENT);
	ui_draw_number((int)net_rx_count(), ui_scaled_pos(UI_POS_MIDDLE | UI_POS_CENTER, vec2i(x + 60, y)), UI_SIZE_8, UI_COLOR_DEFAULT);

	ui_draw_text_centered("PRESS ESC TO GO BACK", ui_scaled_pos(UI_POS_BOTTOM | UI_POS_CENTER, vec2i(0, -36)), UI_SIZE_8, UI_COLOR_DEFAULT);
}
