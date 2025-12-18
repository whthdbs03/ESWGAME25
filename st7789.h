#pragma once

#include <bcm2835.h>
#include <stdio.h>

// Pin definitions (Adjust these for your specific hardware)
#define TFT_CS      8
#define TFT_DC      25
#define TFT_RST     24

#define ST7789_TFTWIDTH  240
#define ST7789_TFTHEIGHT 240

// ST7789 commands
#define ST7789_NOP     0x00
#define ST7789_SWRESET 0x01
#define ST7789_SLPOUT  0x11
#define ST7789_DISPON  0x29
#define ST7789_CASET   0x2A
#define ST7789_RASET   0x2B
#define ST7789_RAMWR   0x2C
#define ST7789_MADCTL  0x36
#define ST7789_COLMOD  0x3A

void writeCommand(uint8_t cmd);

void writeData(uint8_t data);

void st7789_init();

void st7789_fillScreen(uint16_t color);

void st7789_fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
