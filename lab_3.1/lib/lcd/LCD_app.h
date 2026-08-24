#ifndef __LCD_APP_H_
#define __LCD_APP_H_

#include "LCD_utils.hpp"
#include "DEV_Config.h"
#include "LCD_Touch.h"

#define BUTTON_W 200
#define BUTTON_H 40
#define MARGIN 10
#define MARGIN_2 2*MARGIN

#define BUTTON_X0 (LCD_W / 2 - BUTTON_W / 2)
#define BUTTON_Y0 (LCD_H / 2 - (BUTTON_H / 2) + 30)

#define CORE1_EXIT_FLAG 0xDEADBEEF


void LCD_screen_init(LCD_SCAN_DIR lcd_scan_dir, char* title);
void TP_DrawBoard(void);

#endif