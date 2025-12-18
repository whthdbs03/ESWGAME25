#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <bcm2835.h>
#include "st7789.h"
#include <SDL2/SDL.h>

// ===== 핀 (BCM GPIO 번호) =====
#define PIN_UP     17
#define PIN_DOWN   22
#define PIN_LEFT   27
#define PIN_RIGHT  23
#define PIN_CENTER 4
#define PIN_A      6   // 보드 표기 #6 가정
#define PIN_B      5   // 보드 표기 #5 가정

// ===== 화면/격자 =====
#define CELL 10
#define GRID_W (ST7789_TFTWIDTH  / CELL)  // 24
#define GRID_H (ST7789_TFTHEIGHT / CELL)  // 24

// RGB565
#define C_BLACK  0x0000
#define C_WHITE  0xFFFF
#define C_RED    0xF800
#define C_GREEN  0x07E0
#define C_BLUE   0x001F
#define C_YELLOW 0xFFE0
#define FOOD_COUNT 5   // ← 기본 먹이 개수 (원하면 8, 10도 가능)
#define HUD_ROWS 1               // 점수줄 1줄은 플레이 금지(벽)
#define CLEAR_SCORE GRID_W       // 24점이면 클리어

typedef struct { uint8_t x, y; } Point;
typedef enum { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT } Dir;
typedef enum { ST_MENU, ST_PLAY, ST_PAUSE, ST_GAMEOVER, ST_CLEAR } GameState;

static GameState state = ST_MENU;

static Point snake[GRID_W * GRID_H];
static int snake_len = 0;
static Dir dir = DIR_RIGHT;
static Point foods[FOOD_COUNT];
static int score = 0;

static inline int pressed(uint8_t pin) { return bcm2835_gpio_lev(pin) == LOW; } // PUD_UP
static inline int eq(Point a, Point b) { return a.x==b.x && a.y==b.y; }

static SDL_Window* win = NULL;
static SDL_Renderer* ren = NULL;
static SDL_Texture* tex = NULL;
static uint32_t mirror_rgba[ST7789_TFTWIDTH * ST7789_TFTHEIGHT];

static uint32_t rgb565_to_rgba8888(uint16_t c) {
    uint8_t r = (c >> 11) & 0x1F;
    uint8_t g = (c >> 5)  & 0x3F;
    uint8_t b = (c >> 0)  & 0x1F;
    // 5/6bit -> 8bit 확장
    r = (r << 3) | (r >> 2);
    g = (g << 2) | (g >> 4);
    b = (b << 3) | (b >> 2);
    return (0xFFu<<24) | (r<<16) | (g<<8) | b; // ARGB/RGBA는 텍스처 포맷에 맞춤
}

static int mirror_init(void) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) return 0;

    win = SDL_CreateWindow("SNAKE MIRROR",
                           SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                           ST7789_TFTWIDTH*2, ST7789_TFTHEIGHT*2, 0);
    if (!win) return 0;

    ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    if (!ren) return 0;

    tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888,
                            SDL_TEXTUREACCESS_STREAMING,
                            ST7789_TFTWIDTH, ST7789_TFTHEIGHT);
    if (!tex) return 0;

    return 1;
}

static void mirror_quit(void) {
    if (tex) SDL_DestroyTexture(tex);
    if (ren) SDL_DestroyRenderer(ren);
    if (win) SDL_DestroyWindow(win);
    SDL_Quit();
}

static void mirror_fillRect(int x, int y, int w, int h, uint16_t color) {
    if (x < 0 || y < 0) return;
    if (x + w > ST7789_TFTWIDTH)  w = ST7789_TFTWIDTH - x;
    if (y + h > ST7789_TFTHEIGHT) h = ST7789_TFTHEIGHT - y;

    uint32_t rgba = rgb565_to_rgba8888(color);
    for (int yy = y; yy < y + h; yy++) {
        uint32_t* row = &mirror_rgba[yy * ST7789_TFTWIDTH];
        for (int xx = x; xx < x + w; xx++) row[xx] = rgba;
    }
}

static void mirror_present(void) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            // 창 닫으면 프로그램 종료하고 싶으면 여기서 exit(0) 해도 됨
        }
    }
    SDL_UpdateTexture(tex, NULL, mirror_rgba, ST7789_TFTWIDTH * sizeof(uint32_t));
    SDL_RenderClear(ren);
    SDL_RenderCopy(ren, tex, NULL, NULL);
    SDL_RenderPresent(ren);
}

static void draw_rect_both(int x, int y, int w, int h, uint16_t color) {
    st7789_fillRect(x, y, w, h, color);      // TFT
    mirror_fillRect(x, y, w, h, color);      // HDMI 창
}

typedef struct {
    char ch;
    uint8_t rows[7]; // 5-bit used
} Glyph5x7;

static inline void dot(int x, int y, int s, uint16_t color) {
    // (x,y)는 픽셀 좌표, s는 스케일(2면 2x2)
    // st7789_fillRect((uint16_t)x, (uint16_t)y, (uint16_t)s, (uint16_t)s, color);
    draw_rect_both((uint16_t)x, (uint16_t)y, (uint16_t)s, (uint16_t)s, color);
}

static const Glyph5x7 FONT[] = {
    {' ', {0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
    {'!', {0x04,0x04,0x04,0x04,0x04,0x00,0x04}},

    {'0', {0x1E,0x21,0x23,0x25,0x29,0x31,0x1E}},
    {'1', {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E}},
    {'2', {0x1E,0x21,0x01,0x02,0x04,0x08,0x3F}},
    {'3', {0x1E,0x21,0x01,0x0E,0x01,0x21,0x1E}},
    {'4', {0x02,0x06,0x0A,0x12,0x3F,0x02,0x02}},
    {'5', {0x3F,0x20,0x3E,0x01,0x01,0x21,0x1E}},
    {'6', {0x0E,0x10,0x20,0x3E,0x21,0x21,0x1E}},
    {'7', {0x3F,0x01,0x02,0x04,0x08,0x08,0x08}},
    {'8', {0x1E,0x21,0x21,0x1E,0x21,0x21,0x1E}},
    {'9', {0x1E,0x21,0x21,0x1F,0x01,0x02,0x1C}},

    {'A', {0x1E,0x21,0x21,0x3F,0x21,0x21,0x21}},
    {'B', {0x3E,0x21,0x21,0x3E,0x21,0x21,0x3E}},
    {'C', {0x1E,0x21,0x20,0x20,0x20,0x21,0x1E}},
    {'D', {0x3E,0x21,0x21,0x21,0x21,0x21,0x3E}},
    {'E', {0x3F,0x20,0x20,0x3E,0x20,0x20,0x3F}},
    {'F', {0x3F,0x20,0x20,0x3E,0x20,0x20,0x20}},
    {'G', {0x1E,0x21,0x20,0x27,0x21,0x21,0x1E}},
    {'H', {0x21,0x21,0x21,0x3F,0x21,0x21,0x21}},
    {'I', {0x1F,0x04,0x04,0x04,0x04,0x04,0x1F}},
    {'J', {0x0F,0x02,0x02,0x02,0x02,0x22,0x1C}},
    {'K', {0x21,0x22,0x24,0x38,0x24,0x22,0x21}},
    {'L', {0x20,0x20,0x20,0x20,0x20,0x20,0x3F}},
    {'M', {0x21,0x33,0x2D,0x21,0x21,0x21,0x21}},
    {'N', {0x21,0x31,0x29,0x25,0x23,0x21,0x21}},
    {'O', {0x1E,0x21,0x21,0x21,0x21,0x21,0x1E}},
    {'P', {0x3E,0x21,0x21,0x3E,0x20,0x20,0x20}},
    {'Q', {0x1E,0x21,0x21,0x21,0x25,0x22,0x1D}},
    {'R', {0x3E,0x21,0x21,0x3E,0x24,0x22,0x21}},
    {'S', {0x1F,0x20,0x20,0x1E,0x01,0x01,0x3E}},
    {'T', {0x3F,0x04,0x04,0x04,0x04,0x04,0x04}},
    {'U', {0x21,0x21,0x21,0x21,0x21,0x21,0x1E}},
    {'V', {0x21,0x21,0x21,0x21,0x21,0x12,0x0C}},
    {'W', {0x21,0x21,0x21,0x21,0x2D,0x33,0x21}},
    {'X', {0x21,0x21,0x12,0x0C,0x12,0x21,0x21}},
    {'Y', {0x21,0x21,0x12,0x0C,0x04,0x04,0x04}},
    {'Z', {0x3F,0x01,0x02,0x04,0x08,0x10,0x3F}},
};

static const Glyph5x7* get_glyph(char c) {
    // 대문자만 쓸 거라 소문자 들어오면 대문자로 바꿔도 됨(옵션)
    if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
    for (unsigned i = 0; i < sizeof(FONT)/sizeof(FONT[0]); i++) {
        if (FONT[i].ch == c) return &FONT[i];
    }
    return &FONT[0]; // 모르면 공백
}
static void draw_cell(uint8_t gx, uint8_t gy, uint16_t color) {
    if (gx >= GRID_W || gy >= GRID_H) return;
    draw_rect_both(px, py, CELL, CELL, color);
}
static void draw_char_5x7_px(int px, int py, char c, int s, uint16_t color) {
    const Glyph5x7* g = get_glyph(c);
    for (int row = 0; row < 7; row++) {
        uint8_t bits = g->rows[row];
        for (int col = 0; col < 5; col++) {
            if (bits & (1 << (4 - col))) {
                dot(px + col * s, py + row * s, s, color);
            }
        }
    }
}
static int str_len(const char* s) { int n=0; while (s[n]) n++; return n; }

static void draw_text_center_px(const char* s, int py, int sdot, uint16_t color) {
    int len = str_len(s);
    int char_w = 5 * sdot;
    int gap = 1 * sdot;                // 글자 간 1픽셀(스케일 반영)
    int text_w = len * char_w + (len - 1) * gap;

    int px = (ST7789_TFTWIDTH - text_w) / 2;
    for (int i = 0; i < len; i++) {
        draw_char_5x7_px(px + i * (char_w + gap), py, s[i], sdot, color);
    }
}




static int snake_contains(Point p) {
    for (int i=0;i<snake_len;i++) if (eq(snake[i], p)) return 1;
    return 0;
}

static void spawn_food_at(int idx) {
    Point p;
    do {
        p.x = rand() % GRID_W;
        p.y = HUD_ROWS + (rand() % (GRID_H - HUD_ROWS));
    } while (snake_contains(p));
    foods[idx] = p;
    draw_cell(p.x, p.y, C_YELLOW);
}


static void render_score_bar(void) {
    // 상단 한 줄에 점수 막대(최대 24)
    int bar = score;
    if (bar > GRID_W) bar = GRID_W;
    for (int x=0; x<GRID_W; x++) draw_cell(x, 0, (x < bar) ? C_BLUE : C_BLACK);
}
static void clear_screen_ui(void) {
    st7789_fillScreen(C_GREEN);

    int sdot = 4;
    int start_y = (ST7789_TFTHEIGHT - 7*sdot) / 2;

    draw_text_center_px("CLEAR!", start_y, sdot, C_WHITE);
}



static void reset_game(void) {
    score = 0;
    snake_len = 3;
    dir = DIR_RIGHT;

    uint8_t sy = HUD_ROWS + (GRID_H - HUD_ROWS) / 2;

    snake[0] = (Point){ GRID_W/2, sy };
    snake[1] = (Point){ (uint8_t)(GRID_W/2 - 1), sy };
    snake[2] = (Point){ (uint8_t)(GRID_W/2 - 2), sy };

    st7789_fillScreen(C_BLACK);
    for (int i=0;i<snake_len;i++) draw_cell(snake[i].x, snake[i].y, C_GREEN);

    for (int i = 0; i < FOOD_COUNT; i++) {
        spawn_food_at(i);
    }
    render_score_bar();
}

static void menu_screen(void) {
    st7789_fillScreen(C_BLUE);

    int sdot = 4;
    int line_h = 7*sdot + 6;
    int start_y = (ST7789_TFTHEIGHT - line_h*2) / 2;

    draw_text_center_px("SNAKE", start_y, sdot, C_WHITE);   // 필요 글자 없으면 START만
    draw_text_center_px("START", start_y + line_h, sdot, C_WHITE);
}



static void gameover_screen(void) {
    st7789_fillScreen(C_RED);

    int sdot = 4;         // 글자 크기: 3~4 추천 (4면 잘 보임)
    int line_h = 7*sdot + 6;

    int total_h = line_h * 2;
    int start_y = (ST7789_TFTHEIGHT - total_h) / 2;

    draw_text_center_px("GAME", start_y, sdot, C_WHITE);
    draw_text_center_px("OVER", start_y + line_h, sdot, C_WHITE);
}


static void handle_input(void) {

    if (state == ST_MENU) {
        if (pressed(PIN_CENTER)) {
            bcm2835_delay(200); // 디바운스
            reset_game();
            state = ST_PLAY;
        }
        return;
    }

    if (state == ST_GAMEOVER) {
        if (pressed(PIN_CENTER)) {
            bcm2835_delay(200);
            state = ST_MENU;
            menu_screen();
        }
        return;
    }

    if (state == ST_CLEAR) {
        if (pressed(PIN_CENTER)) {
            bcm2835_delay(200);
            state = ST_MENU;
            menu_screen();
        }
        return;
    }

    // A 버튼: 일시정지 토글
    if (pressed(PIN_A)) {
        bcm2835_delay(200);
        if (state == ST_PLAY) state = ST_PAUSE;
        else if (state == ST_PAUSE) state = ST_PLAY;
        return;
    }

    // B 버튼: 메뉴로
    if (pressed(PIN_B)) {
        bcm2835_delay(200);
        state = ST_MENU;
        menu_screen();
        return;
    }

    if (state != ST_PLAY) return;

    // 방향 입력 (반대방향 금지)
    if (pressed(PIN_UP) && dir != DIR_DOWN) {
        dir = DIR_UP; bcm2835_delay(120);
    }
    else if (pressed(PIN_DOWN) && dir != DIR_UP) {
        dir = DIR_DOWN; bcm2835_delay(120);
    }
    else if (pressed(PIN_LEFT) && dir != DIR_RIGHT) {
        dir = DIR_LEFT; bcm2835_delay(120);
    }
    else if (pressed(PIN_RIGHT) && dir != DIR_LEFT) {
        dir = DIR_RIGHT; bcm2835_delay(120);
    }
}

static void tick_move(void) {
    Point head = snake[0];
    Point nh = head;

    if (dir == DIR_UP) nh.y--;
    else if (dir == DIR_DOWN) nh.y++;
    else if (dir == DIR_LEFT) nh.x--;
    else if (dir == DIR_RIGHT) nh.x++;

    // 벽 충돌
    if (nh.x >= GRID_W || nh.y >= GRID_H || nh.y < HUD_ROWS) {
        state = ST_GAMEOVER;
        gameover_screen();
        return;
    }
    // 몸 충돌
    if (snake_contains(nh)) {
        state = ST_GAMEOVER;
        gameover_screen();
        return;
    }

    int ate = -1;
    for (int i = 0; i < FOOD_COUNT; i++) {
        if (eq(nh, foods[i])) {
            ate = i;
            break;
        }
    }

    if (ate >= 0) score++;
    if (score >= CLEAR_SCORE) {
        state = ST_CLEAR;
        clear_screen_ui();
        return;
    }


    // 꼬리 처리(먹었으면 길이+1)
    Point tail = snake[snake_len - 1];
    if (ate<0) draw_cell(tail.x, tail.y, C_BLACK);
    else snake_len++;

    // shift
    for (int i = snake_len - 1; i > 0; i--) snake[i] = snake[i-1];
    snake[0] = nh;

    draw_cell(nh.x, nh.y, C_GREEN);

    if (ate >= 0) {
        spawn_food_at(ate);  // 먹은 자리만 다시 생성
    }
    render_score_bar();
}

static void setup_inputs(void) {
    uint8_t pins[] = {PIN_UP,PIN_DOWN,PIN_LEFT,PIN_RIGHT,PIN_CENTER,PIN_A,PIN_B};
    for (int i=0;i<(int)(sizeof(pins)/sizeof(pins[0]));i++) {
        bcm2835_gpio_fsel(pins[i], BCM2835_GPIO_FSEL_INPT);
        bcm2835_gpio_set_pud(pins[i], BCM2835_GPIO_PUD_UP);
    }
}

int main(void) {
    if (!bcm2835_init()) {
        printf("bcm2835_init failed. Are you running as root?\n");
        return 1;
    }

    // TFT 핀은 st7789.h의 TFT_DC/TFT_RST 사용
    bcm2835_gpio_fsel(TFT_DC,  BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(TFT_RST, BCM2835_GPIO_FSEL_OUTP);

    // 백라이트(26) ON (너 설정 그대로)
    bcm2835_gpio_fsel(26, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_set(26);

    st7789_init();
    if (!mirror_init()) {
        printf("SDL mirror init failed\n");
    }
    setup_inputs();
    srand((unsigned)time(NULL));

    menu_screen();

    const uint32_t tick_ms = 120;

    while (1) {
        handle_input();

        if (state == ST_PLAY) {
            tick_move();
            bcm2835_delay(120); // 게임 속도
        } else {
            bcm2835_delay(50);
        }

        mirror_present();

    }

    bcm2835_spi_end();
    bcm2835_close();
    return 0;
}
