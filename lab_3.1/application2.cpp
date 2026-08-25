#include <stdio.h>
#include <vector>
#include <cstring> 

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/sync.h"
#include "hardware/watchdog.h"
#include "hardware/sync.h"

extern "C" {
	#include "DEV_Config.h"
}
#include "LCD_app.h"
#include "context.h"

// run core0 loop that displays UI and handle user interaction
void core1_entry() {
    while(true) {
        LCD_SetBackLight(1000);
        TP_DrawBoard();
    }
}

int main(void)
{
    System_Init();

    sleep_ms(3000);

	LCD_SCAN_DIR  lcd_scan_dir = SCAN_DIR_DFT;
	LCD_screen_init(lcd_scan_dir, "Application 2");

    // run core1 loop that handles user interface
    multicore_launch_core1(core1_entry);

	while(1) {
        uint32_t g = multicore_fifo_pop_blocking();
        if (g == CORE1_EXIT_FLAG) {
            break; // Exit the loop if the exit flag is received
        }
	}

    printf("Exiting core1 loop\n");
    multicore_reset_core1();
    sleep_ms(1000);

    printf("Jumping to the app1...\n");
    jump_to_image(APP1_OFFSET);
	return 0;
}