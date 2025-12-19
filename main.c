#include <stdio.h>
#include <bcm2835.h>

#include "config.h"
#include "game.h"
#include "render.h"
#include "st7789.h"

int main(void) {
    if (!bcm2835_init()) { // bcm2835 - 라즈베리파이의 GPIO/SPI 등 제어 라이브러리 초기화
        printf("bcm2835_init failed. Are you running as root?\n");
        return 1;
    }

    // TFT pins defined in st7789.h (TFT_DC/TFT_RST)
    // 해당  GPIO 핀을 출력으로 설정
    bcm2835_gpio_fsel(TFT_DC,  BCM2835_GPIO_FSEL_OUTP); // 데이터/명령 선택 핀
    bcm2835_gpio_fsel(TFT_RST, BCM2835_GPIO_FSEL_OUTP); // 리셋 핀

    // Backlight (GPIO 26) ON
    bcm2835_gpio_fsel(26, BCM2835_GPIO_FSEL_OUTP); // 백라이트 핀을 출력으로 설정
    bcm2835_gpio_set(26); // 백라이트 켜기

    st7789_init(); // ST7789 TFT 디스플레이 초기화. SPI 설정 및 초기화 명령 전송
    if (!render_init()) { // SDL 미러 렌더러 초기화 - 필수 아님 임의로 추가한 기능
        printf("SDL mirror init failed\n");
    }
    if (!game_init()) { // 게임 초기화
        printf("game_init failed\n");
        return 1;
    }

    game_loop(); // 게임 루프 시작

    bcm2835_spi_end(); // SPI 종료
    render_quit(); // 렌더러 종료
    bcm2835_close(); // bcm2835 라이브러리 종료
    return 0;
}
