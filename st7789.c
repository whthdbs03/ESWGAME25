#include <bcm2835.h>
#include <stdio.h>

#include "st7789.h"

void writeCommand(uint8_t cmd) {
    bcm2835_gpio_clr(TFT_DC);
    bcm2835_spi_transfer(cmd);
}

void writeData(uint8_t data) {
    bcm2835_gpio_set(TFT_DC);
    bcm2835_spi_transfer(data);
}

void st7789_init() {
    bcm2835_spi_begin();
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_8);
    
    // Hardware reset
    bcm2835_gpio_clr(TFT_RST);
    delay(100);
    bcm2835_gpio_set(TFT_RST);
    delay(100);

    writeCommand(ST7789_SWRESET); // Software reset
    delay(150);

    writeCommand(ST7789_SLPOUT);  // Sleep out
    delay(500);

    writeCommand(ST7789_COLMOD);  // Set color mode
    writeData(0x55);              // 16-bit color
    delay(10);

    writeCommand(ST7789_MADCTL);
    writeData(0x00);              // Normal display

    writeCommand(ST7789_CASET);
    writeData(0x00); writeData(0x00); // XSTART = 0
    writeData(ST7789_TFTWIDTH >> 8); writeData(ST7789_TFTWIDTH & 0xFF); // XEND

    writeCommand(ST7789_RASET);
    writeData(0x00); writeData(0x00); // YSTART = 0
    writeData(ST7789_TFTHEIGHT >> 8); writeData(ST7789_TFTHEIGHT & 0xFF); // YEND

    writeCommand(ST7789_DISPON);  // Display on
    delay(100);
}

void st7789_fillScreen(uint16_t color) {
    uint8_t hi = color >> 8, lo = color & 0xFF;
    writeCommand(ST7789_RASET);
    writeData(0); writeData(0);
    writeData(ST7789_TFTHEIGHT >> 8); writeData(ST7789_TFTHEIGHT & 0xFF);
    
    writeCommand(ST7789_CASET);
    writeData(0); writeData(0);
    writeData(ST7789_TFTWIDTH >> 8); writeData(ST7789_TFTWIDTH & 0xFF);
    
    writeCommand(ST7789_RAMWR);
    bcm2835_gpio_set(TFT_DC);

    int i, j;
    for (i = 0; i < ST7789_TFTHEIGHT; i++) {
        for (j = 0; j < ST7789_TFTWIDTH; j++) {
            bcm2835_spi_transfer(hi);
            bcm2835_spi_transfer(lo);
        }
    }
}

void st7789_fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) { // 사각형 그리기
    if (x >= ST7789_TFTWIDTH || y >= ST7789_TFTHEIGHT) return; // 범위 밖이면 무시
    if (x + w > ST7789_TFTWIDTH)  w = ST7789_TFTWIDTH - x; // 너비 조정
    if (y + h > ST7789_TFTHEIGHT) h = ST7789_TFTHEIGHT - y; // 높이 조정

    uint8_t hi = color >> 8, lo = color & 0xFF; // 색상 분리

    writeCommand(ST7789_CASET); // 열 주소 설정
    writeData(x >> 8); writeData(x & 0xFF); // 시작 열
    uint16_t x2 = x + w - 1; // 끝 열
    writeData(x2 >> 8); writeData(x2 & 0xFF); // 끝 열

    writeCommand(ST7789_RASET); // 행 주소 설정
    writeData(y >> 8); writeData(y & 0xFF); // 시작 행
    uint16_t y2 = y + h - 1; // 끝 행
    writeData(y2 >> 8); writeData(y2 & 0xFF); // 끝 행

    writeCommand(ST7789_RAMWR); // 메모리 쓰기 시작
    bcm2835_gpio_set(TFT_DC); // 데이터 모드

    uint32_t n = (uint32_t)w * (uint32_t)h; // 총 픽셀 수
    for (uint32_t i = 0; i < n; i++) { // 픽셀 데이터 전송
        bcm2835_spi_transfer(hi); // 상위 바이트
        bcm2835_spi_transfer(lo); // 하위 바이트
    }
}
