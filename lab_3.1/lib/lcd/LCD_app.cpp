#include <string.h>
#include <stdio.h>
#include <vector>
#include <cstdint>
#include "pico/multicore.h"

#include "LCD_app.h"

static TextField* text_field;
static Button* switch_app_button;
static TP_DEV sTP_DEV;
static TP_DRAW sTP_Draw;


void reset_display(char* app_name) {
    LCD_Clear(LCD_BACKGROUND);

    TP_DrawHeader(app_name);

    switch_app_button->draw();
}


void on_switch_app_button_pressed(void)
{
    printf("Switching app\n");
    LCD_Clear(LCD_BACKGROUND);
    int screen_center_x = LCD_W / 2;
    int screen_center_y = LCD_H / 2;
    GUI_DisString_EN(screen_center_x - (screen_center_x / 2), screen_center_y, "Button pressed!", &Font24, WHITE, BLACK);
    GUI_DisString_EN(screen_center_x - (screen_center_x / 2), screen_center_y + 26, "Goodbye!", &Font20, WHITE, BLACK);
    sleep_ms(1000);
    multicore_fifo_push_blocking(CORE1_EXIT_FLAG);
}

void TP_DrawBoard(void)
{
    TP_Scan(0);
    if (sTP_DEV.chStatus & TP_PRESS_DOWN)
    { 
        spi_init(SPI_PORT, 10000000);
        sTP_Draw.Color = BLACK;
        if (switch_app_button->is_pressed(sTP_Draw.Xpoint, sTP_Draw.Ypoint))
        { 
            printf("Switch app button pressed\n");
            switch_app_button->call_cb();
        }
        spi_init(SPI_PORT, 5000000);
    }
    SPI4W_Write_Byte(0xFF);
}


void LCD_screen_init(LCD_SCAN_DIR lcd_scan_dir, char* title)
{
    LCD_Init(lcd_scan_dir, 1000);
    TP_Init(lcd_scan_dir, &sTP_DEV, &sTP_Draw);

    TP_GetAdFac();

    int screen_top_x = LCD_W / 2;
    int screen_top_y = LCD_H / 3;

    switch_app_button = new Button(BUTTON_X0, BUTTON_Y0, BUTTON_W, BUTTON_H, "SWITCH APP", DOT_PIXEL_2X2, RED, WHITE, 24, 13);
    switch_app_button->set_callback(on_switch_app_button_pressed);

    reset_display(title);
}
