#include "game.h"

#include <stdlib.h>
#include <time.h>

#include <bcm2835.h>

#include "config.h"
#include "font5x7.h"
#include "render.h"

static GameState state = ST_MENU;
static Point snake[GRID_W * GRID_H]; // 24 * 24 그리드 최대 크기
static int snake_len = 0;
static Dir dir = DIR_RIGHT; // 초기 이동 방향
static Point foods[FOOD_COUNT]; // 음식마다의 좌표
static int score = 0;

static inline int pressed(uint8_t pin) { return bcm2835_gpio_lev(pin) == LOW; } // 버튼이 눌렸는지 확인
static inline int eq(Point a, Point b) { return a.x == b.x && a.y == b.y; } // 두 점이 같은지 확인

static void setup_inputs(void) { // 입력 초기화
    uint8_t pins[] = {PIN_UP, PIN_DOWN, PIN_LEFT, PIN_RIGHT, PIN_CENTER, PIN_A, PIN_B};
    for (int i = 0; i < (int)(sizeof(pins) / sizeof(pins[0])); i++) {
        bcm2835_gpio_fsel(pins[i], BCM2835_GPIO_FSEL_INPT); // 입력으로 설정
        bcm2835_gpio_set_pud(pins[i], BCM2835_GPIO_PUD_UP); // 풀업을 켜서 기본값을 HIGH로 설정
    }
}

static int snake_contains(Point p) { // 뱀이 특정 좌표 p를 포함하는지 확인
    for (int i = 0; i < snake_len; i++) if (eq(snake[i], p)) return 1;
    return 0;
}

static void spawn_food_at(int idx) { // 음식 생성
    Point p;
    do {
        p.x = rand() % GRID_W;
        p.y = HUD_ROWS + (rand() % (GRID_H - HUD_ROWS)); // HUD_ROWS(점수벽) 이후 영역에 생성
    } while (snake_contains(p)); // 뱀과 겹치지 않도록 함
    foods[idx] = p; // 음식 좌표 저장
    draw_cell(p.x, p.y, C_YELLOW); // 음식 그리기
}

static void render_score_bar(void) { // 점수 바 렌더링
    int bar = score;
    if (bar > GRID_W) bar = GRID_W; // 최대 길이 제한 현재 24
    for (int x = 0; x < GRID_W; x++) draw_cell((uint8_t)x, 0, (x < bar) ? C_BLUE : C_BLACK); // 파란색: 점수, 검은색: 빈 공간
    // 이렇게 매 틱마다 그려야 혹시나 음식/뱀이 점수 바와 겹칠 때 덮어쓰는 것을 방지할 수 있음
}

static void clear_screen_ui(void) { // 클리어 화면
    fill_screen_both(C_GREEN); // 배경을 초록색으로 채움 실제 배경과 모니터미러링둘다 

    int sdot = 4; // 문자 크기
    int start_y = (ST7789_TFTHEIGHT - 7 * sdot) / 2;

    draw_text_center_px("CLEAR!", start_y, sdot, C_WHITE); // 중앙에 "CLEAR!" 텍스트 그리기
}

static void reset_game(void) { // 게임 리셋
    score = 0;
    snake_len = 3;
    dir = DIR_RIGHT;

    uint8_t sy = HUD_ROWS + (GRID_H - HUD_ROWS) / 2;

    // 뱀 초기 위치 설정 - 화면 중앙 가로줄 3칸
    snake[0] = (Point){ GRID_W / 2, sy };
    snake[1] = (Point){ (uint8_t)(GRID_W / 2 - 1), sy };
    snake[2] = (Point){ (uint8_t)(GRID_W / 2 - 2), sy };

    fill_screen_both(C_BLACK); // 화면 전체를 검은색으로 채움
    for (int i = 0; i < snake_len; i++) draw_cell(snake[i].x, snake[i].y, C_GREEN); // 뱀 그리기

    for (int i = 0; i < FOOD_COUNT; i++) { // 음식 생성
        spawn_food_at(i);
    }
    render_score_bar(); //  점수 바 렌더링
}

static void menu_screen(void) { // 메뉴 화면
    fill_screen_both(C_BLUE); // 배경을 파란색으로 채움

    int sdot = 4;
    int line_h = 7 * sdot + 6;
    int start_y = (ST7789_TFTHEIGHT - line_h * 2) / 2;

    draw_text_center_px("SNAKE", start_y, sdot, C_WHITE);
    draw_text_center_px("START", start_y + line_h, sdot, C_WHITE);
}

static void gameover_screen(void) { // 게임 오버 화면
    fill_screen_both(C_RED); // 배경을 빨간색으로 채움

    int sdot = 4;
    int line_h = 7 * sdot + 6;

    int total_h = line_h * 2;
    int start_y = (ST7789_TFTHEIGHT - total_h) / 2;

    draw_text_center_px("GAME", start_y, sdot, C_WHITE);
    draw_text_center_px("OVER", start_y + line_h, sdot, C_WHITE);
}

static void handle_input(void) { // 상태 별로 입력 처리
    if (state == ST_MENU) { // 메뉴 상태
        if (pressed(PIN_CENTER)) { // 중앙 버튼이 눌렸으면
            bcm2835_delay(200); // 디바운스 딜레이
            reset_game(); // 게임 리셋
            state = ST_PLAY; // 게임 상태로 전환
        }
        return;
    }

    if (state == ST_GAMEOVER) { // 게임 오버 상태
        if (pressed(PIN_CENTER)) { // 중앙 버튼이 눌렸으면
            bcm2835_delay(200); // 디바운스 딜레이
            state = ST_MENU; //  메뉴 상태로 전환
            menu_screen(); // 메뉴 화면 표시
        }
        return;
    }

    if (state == ST_CLEAR) { // 클리어 상태
        if (pressed(PIN_CENTER)) { // 중앙 버튼이 눌렸으면
            bcm2835_delay(200); // 디바운스 딜레이
            state = ST_MENU; // 메뉴 상태로 전환
            menu_screen(); // 메뉴 화면 표시
        }
        return;
    }

    if (pressed(PIN_A)) { // A 버튼이 눌렸으면
        bcm2835_delay(200); // 디바운스 딜레이
        if (state == ST_PLAY) state = ST_PAUSE; // 일시정지 상태로 전환
        else if (state == ST_PAUSE) state = ST_PLAY; // 플레이 상태로 전환
        return;
    }
 
    if (pressed(PIN_B)) { // B 버튼이 눌렸으면
        bcm2835_delay(200); // 디바운스 딜레이
        state = ST_MENU; // 메뉴 상태로 전환
        menu_screen(); // 메뉴 화면 표시
        return;
    }

    if (state != ST_PLAY) return; // 플레이 상태가 아니면 방향키 무시

    if (pressed(PIN_UP) && dir != DIR_DOWN) { // 위 버튼이 눌렸고 현재 방향이 아래가 아니면
        dir = DIR_UP; bcm2835_delay(120); // 방향을 위로 변경, 딜레이로 너무 빠른 입력 방지
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

static void tick_move(void) { // 뱀 이동 틱
    Point head = snake[0]; // 현재 머리 위치
    Point nh = head; // 새로운 머리 위치 계산

    if (dir == DIR_UP) nh.y--; // 위로 이동
    else if (dir == DIR_DOWN) nh.y++;
    else if (dir == DIR_LEFT) nh.x--;
    else if (dir == DIR_RIGHT) nh.x++;

    if (nh.x >= GRID_W || nh.y >= GRID_H || nh.y < HUD_ROWS) { // 벽 충돌 검사
        state = ST_GAMEOVER;
        gameover_screen();
        return;
    }
    if (snake_contains(nh)) { // 자기 몸과 충돌 검사
        state = ST_GAMEOVER;
        gameover_screen();
        return;
    }

    // 음식 섭취 검사
    int ate = -1;
    for (int i = 0; i < FOOD_COUNT; i++) {
        if (eq(nh, foods[i])) {
            ate = i;
            break;
        }
    }

    // 점수 증가 및 클리어 검사
    if (ate >= 0) score++;
    if (score >= CLEAR_SCORE) {
        state = ST_CLEAR;
        clear_screen_ui();
        return;
    }

    // 뱀 이동 처리
    Point tail = snake[snake_len - 1];
    if (ate < 0) draw_cell(tail.x, tail.y, C_BLACK); // 꼬리 지우기
    else snake_len++; // 음식 먹었으면 길이 증가

    for (int i = snake_len - 1; i > 0; i--) snake[i] = snake[i - 1]; // 몸통 이동
    snake[0] = nh; // 머리 위치 갱신

    draw_cell(nh.x, nh.y, C_GREEN); // 새로운 머리 그리기

    if (ate >= 0) { // 음식 먹었으면
        spawn_food_at(ate); // 새로운 음식 생성
    }
    render_score_bar(); // 점수 바 렌더링
}

// 얘넨 static 아님

int game_init(void) { // 게임 초기화
    setup_inputs();
    srand((unsigned)time(NULL)); // 랜덤 시드 설정 - 먹이랜덤위치
    menu_screen();
    return 1;
}

void game_loop(void) {
    while (1) {
        handle_input();

        if (state == ST_PLAY) {
            tick_move();
            bcm2835_delay(120); // 게임 속도 조절
        } else {
            bcm2835_delay(50);
        }

        render_present();
    }
}
