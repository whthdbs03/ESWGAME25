# ESWGAME25
ESW TFT Joystick Game.

지렁이 밥을 줘요

lbcm2835 라이브러리를 설치해야합니다
sdl2를 설치하면 미러링 가능  
sudo apt update  
sudo apt install -y libsdl2-dev  

gcc -O2 -o snake main.c game.c render.c font5x7.c st7789.c -lbcm2835 -lSDL2  
ㄴsdl2 선택

sudo -E ./snake

## 코드 구조 요약
- `config.h`: 핀 번호, 격자/게임 설정(`CELL`, `GRID_W`, `FOOD_COUNT` 등), 색상 상수 정의.
- `render.h`/`render.c`: SDL 미러 초기화/정리(`render_init`, `render_quit`), 프레임 표시(`render_present`), TFT+SDL 동시 그리기 헬퍼(`fill_screen_both`, `draw_rect_both`, `draw_cell`, `dot`).
- `font5x7.h`/`font5x7.c`: 5x7 글리프 테이블과 텍스트 렌더링(`draw_char_5x7_px`, `draw_text_center_px`, `str_len`).
- `game.h`/`game.c`: 게임 상태/로직 관리(`game_init`, `game_loop`, 입력 처리, 이동·충돌·점수·UI 처리), 스네이크/먹이/점수 상태 보관.
- `main.c`: bcm2835 초기화, TFT/백라이트 설정, `st7789_init`, `render_init`, `game_init` 호출 후 `game_loop` 실행.
