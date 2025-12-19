#include "render.h"

#include <SDL2/SDL.h>

#include "config.h"
#include "st7789.h"

static SDL_Window* win = NULL;
static SDL_Renderer* ren = NULL;
static SDL_Texture* tex = NULL;
static uint32_t mirror_rgba[ST7789_TFTWIDTH * ST7789_TFTHEIGHT];

static uint32_t rgb565_to_rgba8888(uint16_t c) { // RGB565를 ARGB8888로 변환 TFT -> 미러링
    uint8_t r = (c >> 11) & 0x1F;
    uint8_t g = (c >> 5)  & 0x3F;
    uint8_t b = (c >> 0)  & 0x1F;
    r = (r << 3) | (r >> 2);
    g = (g << 2) | (g >> 4);
    b = (b << 3) | (b >> 2);
    return (0xFFu << 24) | (r << 16) | (g << 8) | b;
}

static int mirror_init(void) { // SDL 미러 렌더러 초기화
    if (SDL_Init(SDL_INIT_VIDEO) != 0) return 0;

    win = SDL_CreateWindow("SNAKE MIRROR",
                           SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                           ST7789_TFTWIDTH * 2, ST7789_TFTHEIGHT * 2, 0);
    if (!win) return 0;

    ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    if (!ren) return 0;

    tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888,
                            SDL_TEXTUREACCESS_STREAMING,
                            ST7789_TFTWIDTH, ST7789_TFTHEIGHT);
    if (!tex) return 0;

    return 1;
}

static void mirror_quit(void) { // 렌더러 종료
    if (tex) SDL_DestroyTexture(tex);
    if (ren) SDL_DestroyRenderer(ren);
    if (win) SDL_DestroyWindow(win);
    SDL_Quit();
}

static void mirror_fillRect(int x, int y, int w, int h, uint16_t color) { // 미러링에 사각형 그리기
    if (x < 0 || y < 0) return;
    if (x + w > ST7789_TFTWIDTH)  w = ST7789_TFTWIDTH - x;
    if (y + h > ST7789_TFTHEIGHT) h = ST7789_TFTHEIGHT - y;

    uint32_t rgba = rgb565_to_rgba8888(color);
    for (int yy = y; yy < y + h; yy++) {
        uint32_t* row = &mirror_rgba[yy * ST7789_TFTWIDTH];
        for (int xx = x; xx < x + w; xx++) row[xx] = rgba;
    }
}
 
static void mirror_fillScreen(uint16_t color) { // 미러링에 화면 전체 채우기
    uint32_t rgba = rgb565_to_rgba8888(color);
    for (int i = 0; i < ST7789_TFTWIDTH * ST7789_TFTHEIGHT; i++) {
        mirror_rgba[i] = rgba;
    }
}

int render_init(void) {
    if (!mirror_init()) return 0;
    return 1;
}

void render_quit(void) {
    mirror_quit();
}

int render_present(void) { // 1이면 계속, 0이면 종료 요청
    SDL_Event e;
    while (SDL_PollEvent(&e)) { // q 또는 ESC 누르면 종료
        if (e.type == SDL_QUIT) return 0;
        if (e.type == SDL_KEYDOWN) {
            SDL_Keycode k = e.key.keysym.sym;
            if (k == SDLK_q || k == SDLK_ESCAPE) return 0;
        }
    }
    SDL_UpdateTexture(tex, NULL, mirror_rgba, ST7789_TFTWIDTH * sizeof(uint32_t));
    SDL_RenderClear(ren);
    SDL_RenderCopy(ren, tex, NULL, NULL);
    SDL_RenderPresent(ren);

    return 1;
}

void fill_screen_both(uint16_t color) {
    st7789_fillScreen(color);
    mirror_fillScreen(color);
}

void draw_rect_both(int x, int y, int w, int h, uint16_t color) {
    st7789_fillRect((uint16_t)x, (uint16_t)y, (uint16_t)w, (uint16_t)h, color);
    mirror_fillRect(x, y, w, h, color);
}

void draw_cell(uint8_t gx, uint8_t gy, uint16_t color) {
    int px = gx * CELL; // 격자 좌표를 픽셀 좌표로 변환
    int py = gy * CELL;
    draw_rect_both(px, py, CELL, CELL, color);
}

void dot(int x, int y, int s, uint16_t color) { // 폰트는 5*7 픽셀인데 s배 확대해서 그리기
    draw_rect_both(x, y, s, s, color);
}
