#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/structs/scb.h"
#include "hardware/sync.h"
#include "hardware/structs/timer.h"
#include "pico/platform.h" 

static inline void debug_unpause_timer(void) {
    timer_hw->dbgpause = 0; 
}

int main() {
    stdio_init_all();
    // debug_unpause_timer(); // When debugging, uncomment this line to unpause the timer

    if (cyw43_arch_init_with_country(CYW43_COUNTRY_WORLDWIDE)) {
        printf("Wi-Fi chip init failed\n");
        return -1;
    }

    cyw43_arch_enable_sta_mode();

    int x = 0;
    while (true) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        x++;
        printf("[%d] Hello, world!\n", x);  
        sleep_ms(1000);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        sleep_ms(1000);
    }
}