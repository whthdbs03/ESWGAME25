#pragma once

#include <stdint.h>

typedef struct { 
    char ch; // 뭔 문자인지 
    uint8_t rows[7]; // 글자를 7줄로 나눈 행 데이터 .. ㄱㄹ자 폭이 5픽셀이라 5비트 사용
} Glyph5x7; // 5x7 픽셀 글리프 한 글 자 . .

int str_len(const char* s); // 문자열 길이 반환
void draw_char_5x7_px(int px, int py, char c, int s, uint16_t color); // 문자 그리기
void draw_text_center_px(const char* s, int py, int sdot, uint16_t color); // 중앙 정렬 텍스트 그리기
