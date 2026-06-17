#include "../render.h"
#include "../utils.h"

#include "ui.h"
#include "image.h"

typedef struct {
	vec2i_t offset;
	uint16_t width;
} glyph_t;

typedef struct {
	uint16_t texture;
	uint16_t height;
	glyph_t glyphs[40];
} char_set_t;

int ui_scale = 2;

char_set_t char_set[UI_SIZE_MAX] = {
	[UI_SIZE_16] = {
		.texture = 0,
		.height = 16,
		.glyphs = {
			{{  0,   0}, 13}, {{ 13,   0}, 13}, {{ 26,   0}, 13}, {{ 39,   0}, 13}, {{ 52,   0}, 12}, {{ 64,   0}, 11}, {{ 75,   0}, 14}, {{ 89,   0}, 13},
			{{102,   0},  5}, {{107,   0}, 10}, {{117,   0}, 13}, {{130,   0}, 11}, {{141,   0}, 15}, {{156,   0}, 13}, {{169,   0}, 14}, {{183,   0}, 12},
			{{195,   0}, 14}, {{209,   0}, 13}, {{222,   0}, 12}, {{234,   0}, 11}, {{  0,  16}, 13}, {{ 13,  16}, 12}, {{ 25,  16}, 17}, {{ 42,  16}, 12},
			{{ 54,  16}, 12}, {{ 66,  16}, 11}, {{ 77,  16}, 10}, {{ 87,  16}, 10}, {{ 97,  16}, 10}, {{107,  16}, 10}, {{117,  16}, 10}, {{127,  16}, 10},
			{{137,  16}, 10}, {{147,  16}, 10}, {{157,  16}, 10}, {{167,  16}, 10}, {{177,  16},  6}, {{183,  16},  5}, {{244,  31},  0}, {{244,  31},  0}
		}
	},
	[UI_SIZE_12] = {
		.texture = 0,
		.height = 12,
		.glyphs = {
			{{  0,   0}, 10}, {{ 10,   0}, 10}, {{ 20,   0}, 10}, {{ 30,   0}, 10}, {{ 40,   0},  9}, {{ 49,   0},  8}, {{ 57,   0}, 11}, {{ 68,   0}, 10},
			{{ 78,   0},  4}, {{ 82,   0},  8}, {{ 90,   0}, 10}, {{100,   0},  8}, {{108,   0}, 12}, {{120,   0}, 10}, {{130,   0}, 11}, {{141,   0},  9},
			{{150,   0}, 11}, {{161,   0}, 10}, {{171,   0},  9}, {{180,   0},  8}, {{188,   0}, 10}, {{198,   0},  9}, {{207,   0}, 13}, {{220,   0},  9},
			{{229,   0},  9}, {{238,   0},  8}, {{246,   0},  8}, {{  0,  12},  8}, {{  8,  12},  8}, {{ 16,  12},  8}, {{ 24,  12},  8}, {{ 32,  12},  8},
			{{ 40,  12},  8}, {{ 48,  12},  8}, {{ 56,  12},  8}, {{ 64,  12},  8}, {{ 72,  12},  5}, {{ 77,  12},  4}, {{253,  23},  0}, {{253,  23},  0}
		}
	},
	[UI_SIZE_8] = {
		.texture = 0,
		.height = 8,
		.glyphs = {
			{{  0,   0},  7}, {{  7,   0},  7}, {{ 14,   0},  7}, {{ 21,   0},  7}, {{ 28,   0},  6}, {{ 34,   0},  6}, {{ 40,   0},  7}, {{ 47,   0},  7},
			{{ 54,   0},  2}, {{ 56,   0},  5}, {{ 61,   0},  7}, {{ 68,   0},  6}, {{ 74,   0},  8}, {{ 82,   0},  7}, {{ 89,   0},  7}, {{ 96,   0},  6},
			{{102,   0},  7}, {{109,   0},  7}, {{116,   0},  6}, {{122,   0},  6}, {{128,   0},  7}, {{135,   0},  6}, {{141,   0},  9}, {{150,   0},  6},
			{{156,   0},  6}, {{162,   0},  6}, {{168,   0},  5}, {{173,   0},  5}, {{178,   0},  5}, {{183,   0},  5}, {{188,   0},  5}, {{193,   0},  5},
			{{198,   0},  5}, {{203,   0},  5}, {{208,   0},  5}, {{213,   0},  5}, {{218,   0},  3}, {{221,   0},  2}, {{222,   7},  0}, {{222,   7},  0}
		}
	},
};

uint16_t icon_textures[UI_ICON_MAX];

void ui_load(void) {
	texture_list_t tl = image_get_compressed_textures("wipeout/textures/drfonts.cmp");
	char_set[UI_SIZE_16].texture   = image_get_texture_raw("wipeout/textures/font16.rgba");
	char_set[UI_SIZE_12].texture   = image_get_texture_raw("wipeout/textures/font12.rgba");
	char_set[UI_SIZE_8 ].texture   = image_get_texture_raw("wipeout/textures/font8.rgba");
	icon_textures[UI_ICON_HAND]    = texture_from_list(tl, 3);
	icon_textures[UI_ICON_CONFIRM] = texture_from_list(tl, 5);
	icon_textures[UI_ICON_CANCEL]  = texture_from_list(tl, 6);
	icon_textures[UI_ICON_END]     = texture_from_list(tl, 7);
	icon_textures[UI_ICON_DEL]     = texture_from_list(tl, 8);
	icon_textures[UI_ICON_STAR]    = texture_from_list(tl, 9);
}

int ui_get_scale(void) {
	return ui_scale;
}

void ui_set_scale(int scale) {
	ui_scale = scale;
}


vec2i_t ui_scaled(vec2i_t v) {
	return vec2i(v.x * ui_scale, v.y * ui_scale);
}

vec2i_t ui_scaled_screen(void) {
	return vec2i_mulf(render_size(), ui_scale);
}

vec2i_t ui_scaled_pos(ui_pos_t anchor, vec2i_t offset) {
	vec2i_t pos;
	vec2i_t screen_size = render_size();

	if (flags_is(anchor, UI_POS_LEFT)) {
		pos.x = offset.x * ui_scale;
	}
	else if (flags_is(anchor, UI_POS_CENTER)) {
		pos.x = (screen_size.x >> 1) + offset.x * ui_scale;
	}
	else if (flags_is(anchor, UI_POS_RIGHT)) {
		pos.x = screen_size.x + offset.x * ui_scale;
	}

	if (flags_is(anchor, UI_POS_TOP)) {
		pos.y = offset.y * ui_scale;
	}
	else if (flags_is(anchor, UI_POS_MIDDLE)) {
		pos.y = (screen_size.y >> 1) + offset.y * ui_scale;
	}
	else if (flags_is(anchor, UI_POS_BOTTOM)) {
		pos.y = screen_size.y + offset.y * ui_scale;
	}

	return pos;
}

#define char_to_glyph_index(C) (C >= '0' && C <= '9' ? (C - '0' + 26) : C - 'A')

int ui_char_width(char c, ui_text_size_t size) {
	if (c == ' ') {
		return 8;
	}
	return char_set[size].glyphs[char_to_glyph_index(c)].width;
}

int ui_text_width(const char *text, ui_text_size_t size) {
	int width = 0;
	char_set_t *cs = &char_set[size];

	for (int i = 0; text[i] != 0; i++) {
		width += text[i] != ' '
			? cs->glyphs[char_to_glyph_index(text[i])].width
			: 8;
	}

	return width;
}

int ui_number_width(int num, ui_text_size_t size) {
	char text_buffer[16];
	text_buffer[15] = '\0';

	int i;
	for (i = 14; i > 0; i--) {
		text_buffer[i] = '0' + (num % 10);
		num = num / 10;
		if (num == 0) {
			break;
		}
	}
	return ui_text_width(text_buffer + i, size);
}

void ui_draw_time(float time, vec2i_t pos, ui_text_size_t size, rgba_t color) {
	int msec = time * 1000;
	int tenths = (msec / 100) % 10;
	int secs = (msec / 1000) % 60;
	int mins = msec / (60 * 1000);

	char text_buffer[8];
	text_buffer[0] = '0' + (mins / 10) % 10;
	text_buffer[1] = '0' + mins % 10;
	text_buffer[2] = 'e'; // ":"
	text_buffer[3] = '0' + secs / 10;
	text_buffer[4] = '0' + secs % 10;
	text_buffer[5] = 'f'; // "."
	text_buffer[6] = '0' + tenths;
	text_buffer[7] = '\0';
	ui_draw_text(text_buffer, pos, size, color);
}

void ui_draw_number(int num, vec2i_t pos, ui_text_size_t size, rgba_t color) {
	char text_buffer[16];
	text_buffer[15] = '\0';

	int i;
	for (i = 14; i > 0; i--) {
		text_buffer[i] = '0' + (num % 10);
		num = num / 10;
		if (num == 0) {
			break;
		}
	}
	ui_draw_text(text_buffer + i, pos, size, color);
}

void ui_draw_text(const char *text, vec2i_t pos, ui_text_size_t size, rgba_t color) {
	char_set_t *cs = &char_set[size];

	for (int i = 0; text[i] != 0; i++) {
		if (text[i] != ' ') {
			glyph_t *glyph = &cs->glyphs[char_to_glyph_index(text[i])];
			vec2i_t size = vec2i(glyph->width, cs->height);
			render_push_2d_tile(pos, glyph->offset, size, ui_scaled(size), color, cs->texture);
			pos.x += glyph->width * ui_scale;
		}
		else {
			pos.x += 8 * ui_scale;
		}
	}
}

void ui_draw_image(vec2i_t pos, uint16_t texture) {
	vec2i_t scaled_size = ui_scaled(render_texture_size(texture));
	render_push_2d(pos, scaled_size, rgba(128, 128, 128, 255), texture);
}

void ui_draw_icon(ui_icon_type_t icon, vec2i_t pos, rgba_t color) {
	render_push_2d(pos, ui_scaled(render_texture_size(icon_textures[icon])), color, icon_textures[icon]);
}

void ui_draw_text_centered(const char *text, vec2i_t pos, ui_text_size_t size, rgba_t color) {
	pos.x -= (ui_text_width(text, size) * ui_scale) >> 1;
	ui_draw_text(text, pos, size, color);
}

// Solid/translucent filled rectangle (panels, bars, divider lines) drawn with the
// engine's untextured white quad. pos/size are in final (already-scaled) pixels.
void ui_draw_rect(vec2i_t pos, vec2i_t size, rgba_t color) {
	render_push_2d(pos, size, color, RENDER_NO_TEXTURE);
}
