#include "game.h"

#include <stdlib.h>
#include <time.h>

#include <bcm2835.h>

#include "config.h"
#include "font5x7.h"
#include "render.h"

static GameState state = ST_MENU;

static Point snake[GRID_W * GRID_H];
static int snake_len = 0;
static Dir dir = DIR_RIGHT;
static Point foods[FOOD_COUNT];
static int score = 0;

static inline int pressed(uint8_t pin) { return bcm2835_gpio_lev(pin) == LOW; }
static inline int eq(Point a, Point b) { return a.x == b.x && a.y == b.y; }

static void setup_inputs(void) {
    uint8_t pins[] = {PIN_UP, PIN_DOWN, PIN_LEFT, PIN_RIGHT, PIN_CENTER, PIN_A, PIN_B};
    for (int i = 0; i < (int)(sizeof(pins) / sizeof(pins[0])); i++) {
        bcm2835_gpio_fsel(pins[i], BCM2835_GPIO_FSEL_INPT);
        bcm2835_gpio_set_pud(pins[i], BCM2835_GPIO_PUD_UP);
    }
}

static int snake_contains(Point p) {
    for (int i = 0; i < snake_len; i++) if (eq(snake[i], p)) return 1;
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
    int bar = score;
    if (bar > GRID_W) bar = GRID_W;
    for (int x = 0; x < GRID_W; x++) draw_cell((uint8_t)x, 0, (x < bar) ? C_BLUE : C_BLACK);
}

static void clear_screen_ui(void) {
    fill_screen_both(C_GREEN);

    int sdot = 4;
    int start_y = (ST7789_TFTHEIGHT - 7 * sdot) / 2;

    draw_text_center_px("CLEAR!", start_y, sdot, C_WHITE);
}

static void reset_game(void) {
    score = 0;
    snake_len = 3;
    dir = DIR_RIGHT;

    uint8_t sy = HUD_ROWS + (GRID_H - HUD_ROWS) / 2;

    snake[0] = (Point){ GRID_W / 2, sy };
    snake[1] = (Point){ (uint8_t)(GRID_W / 2 - 1), sy };
    snake[2] = (Point){ (uint8_t)(GRID_W / 2 - 2), sy };

    fill_screen_both(C_BLACK);
    for (int i = 0; i < snake_len; i++) draw_cell(snake[i].x, snake[i].y, C_GREEN);

    for (int i = 0; i < FOOD_COUNT; i++) {
        spawn_food_at(i);
    }
    render_score_bar();
}

static void menu_screen(void) {
    fill_screen_both(C_BLUE);

    int sdot = 4;
    int line_h = 7 * sdot + 6;
    int start_y = (ST7789_TFTHEIGHT - line_h * 2) / 2;

    draw_text_center_px("SNAKE", start_y, sdot, C_WHITE);
    draw_text_center_px("START", start_y + line_h, sdot, C_WHITE);
}

static void gameover_screen(void) {
    fill_screen_both(C_RED);

    int sdot = 4;
    int line_h = 7 * sdot + 6;

    int total_h = line_h * 2;
    int start_y = (ST7789_TFTHEIGHT - total_h) / 2;

    draw_text_center_px("GAME", start_y, sdot, C_WHITE);
    draw_text_center_px("OVER", start_y + line_h, sdot, C_WHITE);
}

static void handle_input(void) {
    if (state == ST_MENU) {
        if (pressed(PIN_CENTER)) {
            bcm2835_delay(200);
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

    if (pressed(PIN_A)) {
        bcm2835_delay(200);
        if (state == ST_PLAY) state = ST_PAUSE;
        else if (state == ST_PAUSE) state = ST_PLAY;
        return;
    }

    if (pressed(PIN_B)) {
        bcm2835_delay(200);
        state = ST_MENU;
        menu_screen();
        return;
    }

    if (state != ST_PLAY) return;

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

    if (nh.x >= GRID_W || nh.y >= GRID_H || nh.y < HUD_ROWS) {
        state = ST_GAMEOVER;
        gameover_screen();
        return;
    }
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

    Point tail = snake[snake_len - 1];
    if (ate < 0) draw_cell(tail.x, tail.y, C_BLACK);
    else snake_len++;

    for (int i = snake_len - 1; i > 0; i--) snake[i] = snake[i - 1];
    snake[0] = nh;

    draw_cell(nh.x, nh.y, C_GREEN);

    if (ate >= 0) {
        spawn_food_at(ate);
    }
    render_score_bar();
}

int game_init(void) {
    setup_inputs();
    srand((unsigned)time(NULL));
    menu_screen();
    return 1;
}

void game_loop(void) {
    while (1) {
        handle_input();

        if (state == ST_PLAY) {
            tick_move();
            bcm2835_delay(120);
        } else {
            bcm2835_delay(50);
        }

        render_present();
    }
}
