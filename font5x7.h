#pragma once

#include <stdint.h>

typedef struct {
    char ch;
    uint8_t rows[7]; // 5-bit used
} Glyph5x7;

int str_len(const char* s);
void draw_char_5x7_px(int px, int py, char c, int s, uint16_t color);
void draw_text_center_px(const char* s, int py, int sdot, uint16_t color);
