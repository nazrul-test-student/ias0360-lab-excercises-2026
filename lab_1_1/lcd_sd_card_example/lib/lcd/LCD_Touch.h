#ifndef __LCD_TOUCH_H_
#define __LCD_TOUCH_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "DEV_Config.h"
#include "LCD_Driver.h"
#include "LCD_GUI.h"
#include <math.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/float.h"
#include "pico/multicore.h"


#define TP_PRESS_DOWN           0x80
#define TP_PRESSED              0x40

#define DATA_READY_FLAG         0xABCDEF01
#define WRITE_FAILED_FLAG       0x12345678
#define TASK_COMPLETE_FLAG      0xDEADBEAF


// ---- Capture area (the black box) ------------------------------------------
#define BOX_X0 100
#define BOX_Y0 50
#define BOX_X1 380   // right  edge (exclusive in our math)
#define BOX_Y1 290   // bottom edge (exclusive in our math)

#define BOX_W (BOX_X1 - BOX_X0)   // 280
#define BOX_H (BOX_Y1 - BOX_Y0)   // 240

//Touch screen structure
typedef struct {
	POINT Xpoint0;
	POINT Ypoint0;
	POINT Xpoint;
	POINT Ypoint;
	uint8_t chStatus;
	uint8_t chType;
	int16_t iXoff;
	int16_t iYoff;
	float fXfac;
	float fYfac;
	//Select the coordinates of the XPT2046 touch \
	  screen relative to what scan direction
	LCD_SCAN_DIR TP_Scan_Dir;
}TP_DEV;

//Brush structure
typedef struct{
	POINT Xpoint;
	POINT Ypoint;
	COLOR Color;
	DOT_PIXEL DotPixel; 
}TP_DRAW;

typedef struct {
	size_t data_len;
	uint8_t *data;
}TP_DATA;

void TP_GetAdFac(void);
void TP_Adjust(void);
void TP_Dialog(void);
void TP_Save(void);
void TP_DrawBoard(void);
void TP_Init(LCD_SCAN_DIR Lcd_ScanDir, TP_DATA *tp_data_ptr, mutex_t *mutex);

#ifdef __cplusplus
}
#endif

#endif
