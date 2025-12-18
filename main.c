#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <bcm2835.h>
#include "st7789.h"

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

typedef struct { uint8_t x, y; } Point;
typedef enum { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT } Dir;
typedef enum { ST_MENU, ST_PLAY, ST_PAUSE, ST_GAMEOVER } GameState;

static GameState state = ST_MENU;

static Point snake[GRID_W * GRID_H];
static int snake_len = 0;
static Dir dir = DIR_RIGHT;
static Point foods[FOOD_COUNT];
static int score = 0;

static inline int pressed(uint8_t pin) { return bcm2835_gpio_lev(pin) == LOW; } // PUD_UP
static inline int eq(Point a, Point b) { return a.x==b.x && a.y==b.y; }

static void draw_cell(uint8_t gx, uint8_t gy, uint16_t color) {
    if (gx >= GRID_W || gy >= GRID_H) return;
    st7789_fillRect(gx * CELL, gy * CELL, CELL, CELL, color);
}

static int snake_contains(Point p) {
    for (int i=0;i<snake_len;i++) if (eq(snake[i], p)) return 1;
    return 0;
}

static void spawn_food_at(int idx) {
    Point p;
    do {
        p.x = rand() % GRID_W;
        p.y = rand() % GRID_H;
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

static void reset_game(void) {
    score = 0;
    snake_len = 3;
    dir = DIR_RIGHT;

    snake[0] = (Point){ GRID_W/2, GRID_H/2 };
    snake[1] = (Point){ (uint8_t)(GRID_W/2 - 1), (uint8_t)(GRID_H/2) };
    snake[2] = (Point){ (uint8_t)(GRID_W/2 - 2), (uint8_t)(GRID_H/2) };

    st7789_fillScreen(C_BLACK);
    for (int i=0;i<snake_len;i++) draw_cell(snake[i].x, snake[i].y, C_GREEN);

    for (int i = 0; i < FOOD_COUNT; i++) {
        spawn_food_at(i);
    }
    render_score_bar();
}

static void menu_screen(void) {
    st7789_fillScreen(C_BLUE);
    draw_cell(GRID_W/2, GRID_H/2, C_GREEN); // "센터 눌러 시작" 대신 표시
}

static void gameover_screen(void) {
    st7789_fillScreen(C_RED);
    // 점수만큼 하단 흰 막대
    int bar = score; if (bar > GRID_W) bar = GRID_W;
    for (int x=0; x<bar; x++) draw_cell(x, GRID_H-1, C_WHITE);
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
    if (nh.x >= GRID_W || nh.y >= GRID_H) {
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


    // 꼬리 처리(먹었으면 길이+1)
    Point tail = snake[snake_len - 1];
    if (!ate) draw_cell(tail.x, tail.y, C_BLACK);
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

    }

    bcm2835_spi_end();
    bcm2835_close();
    return 0;
}
