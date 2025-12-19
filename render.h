#pragma once

#include <stdint.h>

int render_init(void);
void render_quit(void);
void render_present(void);

void fill_screen_both(uint16_t color);
void draw_rect_both(int x, int y, int w, int h, uint16_t color);
void draw_cell(uint8_t gx, uint8_t gy, uint16_t color);
void dot(int x, int y, int s, uint16_t color);
