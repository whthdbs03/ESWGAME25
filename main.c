#include <stdio.h>
#include <bcm2835.h>

#include "config.h"
#include "game.h"
#include "render.h"
#include "st7789.h"

int main(void) {
    if (!bcm2835_init()) {
        printf("bcm2835_init failed. Are you running as root?\n");
        return 1;
    }

    // TFT pins defined in st7789.h (TFT_DC/TFT_RST)
    bcm2835_gpio_fsel(TFT_DC,  BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(TFT_RST, BCM2835_GPIO_FSEL_OUTP);

    // Backlight (GPIO 26) ON
    bcm2835_gpio_fsel(26, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_set(26);

    st7789_init();
    if (!render_init()) {
        printf("SDL mirror init failed\n");
    }
    if (!game_init()) {
        printf("game_init failed\n");
        return 1;
    }

    game_loop();

    bcm2835_spi_end();
    bcm2835_close();
    return 0;
}
