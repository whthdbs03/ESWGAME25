#pragma once

#include <stdint.h>

int render_init(void); // SDL 미러 렌더러 초기화
void render_quit(void); // 렌더러 종료
int render_present(void);   // 1이면 계속, 0이면 종료 요청

void fill_screen_both(uint16_t color); // 화면 전체를 color로 채움 TFT/미러링 둘다
void draw_rect_both(int x, int y, int w, int h, uint16_t color); // 사각형 그리기 TFT/미러링 둘다
void draw_cell(uint8_t gx, uint8_t gy, uint16_t color); // 격자 좌표에 셀 그리기
void dot(int x, int y, int s, uint16_t color); // 점 그리기
