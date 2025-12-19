#pragma once

#include "st7789.h"

// Pin assignments (BCM GPIO numbers)
#define PIN_UP     17
#define PIN_DOWN   22
#define PIN_LEFT   27
#define PIN_RIGHT  23
#define PIN_CENTER 4
#define PIN_A      6
#define PIN_B      5

// Grid configuration
#define CELL 10
#define GRID_W (ST7789_TFTWIDTH  / CELL)
#define GRID_H (ST7789_TFTHEIGHT / CELL)

#define FOOD_COUNT 5
#define HUD_ROWS 1
#define CLEAR_SCORE GRID_W

// RGB565 colors
#define C_BLACK  0x0000
#define C_WHITE  0xFFFF
#define C_RED    0xF800
#define C_GREEN  0x07E0
#define C_BLUE   0x001F
#define C_YELLOW 0xFFE0
