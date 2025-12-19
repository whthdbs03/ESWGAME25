#pragma once

#include <stdint.h>

typedef struct { uint8_t x, y; } Point;
typedef enum { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT } Dir;
typedef enum { ST_MENU, ST_PLAY, ST_PAUSE, ST_GAMEOVER, ST_CLEAR } GameState;

int game_init(void);
void game_loop(void);
