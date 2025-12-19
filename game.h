#pragma once

#include <stdint.h>

typedef struct { uint8_t x, y; } Point; // 격자 좌표 그리드 24*24쓸거임
typedef enum { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT } Dir; // 이동 방향
typedef enum { ST_MENU, ST_PLAY, ST_PAUSE, ST_GAMEOVER, ST_CLEAR } GameState; // 게임 상태

int game_init(void);
void game_loop(void);
