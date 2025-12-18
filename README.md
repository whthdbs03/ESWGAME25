# ESWGAME25
ESW Joystick Game.

지렁이 밥을 줘요

lbcm2835 라이브러리를 설치해야합니다
sdl2를 설치하면 미러링 가능
sudo apt update
sudo apt install -y libsdl2-dev

gcc -O2 -o snake main.c st7789.c -lbcm2835 -lSDL2  
ㄴsdl2 선택

sudo -E ./snake
