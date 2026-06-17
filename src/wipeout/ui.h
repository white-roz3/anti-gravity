#ifndef UI_H
#define UI_H

#include "../types.h"

typedef enum {
	UI_SIZE_16,
	UI_SIZE_12,
	UI_SIZE_8,
	UI_SIZE_MAX
} ui_text_size_t;

// Clean hi-tech palette. NOTE: the GL shader multiplies rgb x2 (render_gl.c), so
// these are pre-halved — the on-screen color is ~2x the rgb below. Alpha is NOT doubled.
#define UI_COLOR_ACCENT  rgba( 46, 116, 134, 255)  // cool azure/cyan  -> ~(92,232,255)
#define UI_COLOR_DEFAULT rgba(128, 128, 128, 255)  // clean white      -> (255,255,255)
#define UI_COLOR_PANEL   rgba(  7,  11,  19, 150)  // dark translucent "glass" panel
#define UI_COLOR_LINE    rgba( 60, 124, 140, 255)  // bright accent divider line
#define UI_COLOR_HILIGHT rgba( 28,  78, 104,  98)  // translucent selection bar

typedef enum {
	UI_ICON_HAND,
	UI_ICON_CONFIRM,
	UI_ICON_CANCEL,
	UI_ICON_END,
	UI_ICON_DEL,
	UI_ICON_STAR,
	UI_ICON_MAX
} ui_icon_type_t;

typedef enum {
	UI_POS_LEFT   = 1 << 0,
	UI_POS_CENTER = 1 << 1,
	UI_POS_RIGHT =  1 << 2,
	UI_POS_TOP =    1 << 3,
	UI_POS_MIDDLE = 1 << 4,
	UI_POS_BOTTOM = 1 << 5,
} ui_pos_t;

void ui_load(void);
void ui_cleanup(void);

int ui_get_scale(void);
void ui_set_scale(int scale);
vec2i_t ui_scaled(vec2i_t v);
vec2i_t ui_scaled_screen(void);
vec2i_t ui_scaled_pos(ui_pos_t anchor, vec2i_t offset);

int ui_char_width(char c, ui_text_size_t size);
int ui_text_width(const char *text, ui_text_size_t size);
int ui_number_width(int num, ui_text_size_t size);

void ui_draw_text(const char *text, vec2i_t pos, ui_text_size_t size, rgba_t color);
void ui_draw_time(float time, vec2i_t pos, ui_text_size_t size, rgba_t color);
void ui_draw_number(int num, vec2i_t pos, ui_text_size_t size, rgba_t color);

void ui_draw_image(vec2i_t pos, uint16_t texture);
void ui_draw_icon(ui_icon_type_t icon, vec2i_t pos, rgba_t color);
void ui_draw_text_centered(const char *text, vec2i_t pos, ui_text_size_t size, rgba_t color);
void ui_draw_rect(vec2i_t pos, vec2i_t size, rgba_t color);

#endif
