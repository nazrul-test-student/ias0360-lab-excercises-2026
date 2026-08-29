#include "LCD_Touch.h"
#include <stdlib.h>
#include <string.h>  
#include <stdio.h>

extern LCD_DIS sLCD_DIS;
extern uint8_t id;
static TP_DEV sTP_DEV;
static TP_DRAW sTP_Draw;


// ---- Capture area (the black box) ------------------------------------------
#define BOX_X0 100
#define BOX_Y0 50
#define BOX_X1 380   // right  edge (exclusive in our math)
#define BOX_Y1 290   // bottom edge (exclusive in our math)

#define BOX_W (BOX_X1 - BOX_X0)   // 280
#define BOX_H (BOX_Y1 - BOX_Y0)   // 240

// Live "shadow" of what has been drawn (0 = empty, 1 = drawn)
static uint8_t sDrawShadow[BOX_H][BOX_W];

// Snapshot taken when SAVE is pressed (this is what you'll write to SD later)
static uint8_t sSavedBitmap[BOX_H][BOX_W];

// Optional: quick accessor if you want to use these elsewhere
uint16_t TP_SavedWidth(void)  { return BOX_W; }
uint16_t TP_SavedHeight(void) { return BOX_H; }
const uint8_t* TP_SavedData(void) { return &sSavedBitmap[0][0]; }

// Helper to record a pixel into the shadow buffer
static inline void Capture_SetPixel(uint16_t x, uint16_t y)
{
    if (x >= BOX_X0 && x < BOX_X1 && y >= BOX_Y0 && y < BOX_Y1) {
        sDrawShadow[y - BOX_Y0][x - BOX_X0] = 1;
    }
}

static void TP_DumpBitmapToSerial(const uint8_t bmp[BOX_H][BOX_W])
{
    // Build and print one line at a time (faster than putchar per pixel)
    char line[BOX_W + 2];           // + '\n' + '\0'
    line[BOX_W]   = '\n';
    line[BOX_W+1] = '\0';

    for (uint16_t y = 0; y < BOX_H; ++y) {
        for (uint16_t x = 0; x < BOX_W; ++x) {
            line[x] = bmp[y][x] ? '1' : '0';
        }
        printf("%s", line);         // prints 280 chars of 0/1, then newline
    }
}

static uint16_t TP_Read_ADC(uint8_t CMD)
{
    uint16_t Data = 0;

    //A cycle of at least 400ns.
    DEV_Digital_Write(TP_CS_PIN, 0);

    SPI4W_Write_Byte(CMD);
    Driver_Delay_us(200);

    //	dont write 0xff, it will block xpt2046
    //Data = SPI4W_Read_Byte(0Xff);
    Data = SPI4W_Read_Byte(0X00);
    Data <<= 8; //7bit
    Data |= SPI4W_Read_Byte(0X00);
    //Data = SPI4W_Read_Byte(0Xff);
    Data >>= 3; //5bit
    DEV_Digital_Write(TP_CS_PIN, 1);
    return Data;
}

#define READ_TIMES 5 //Number of readings
#define LOST_NUM 1   //Discard value
static uint16_t 
TP_Read_ADC_Average(uint8_t Channel_Cmd)
{
    uint8_t i, j;
    uint16_t Read_Buff[READ_TIMES];
    uint16_t Read_Sum = 0, Read_Temp = 0;
    //LCD SPI speed = 3 MHz
    spi_set_baudrate(SPI_PORT, 3000000);
    //Read and save multiple samples
    for (i = 0; i < READ_TIMES; i++)
    {
        Read_Buff[i] = TP_Read_ADC(Channel_Cmd);
        Driver_Delay_us(200);
    }
    //LCD SPI speed = 18 MHz
    spi_set_baudrate(SPI_PORT, 18000000);
    //Sort from small to large
    for (i = 0; i < READ_TIMES - 1; i++)
    {
        for (j = i + 1; j < READ_TIMES; j++)
        {
            if (Read_Buff[i] > Read_Buff[j])
            {
                Read_Temp = Read_Buff[i];
                Read_Buff[i] = Read_Buff[j];
                Read_Buff[j] = Read_Temp;
            }
        }
    }

    //Exclude the largest and the smallest
    for (i = LOST_NUM; i < READ_TIMES - LOST_NUM; i++)
        Read_Sum += Read_Buff[i];

    //Averaging
    Read_Temp = Read_Sum / (READ_TIMES - 2 * LOST_NUM);

    return Read_Temp;
}

static void TP_Read_ADC_XY(uint16_t *pXCh_Adc, uint16_t *pYCh_Adc)
{
    *pXCh_Adc = TP_Read_ADC_Average(0xD0);
    *pYCh_Adc = TP_Read_ADC_Average(0x90);
}

#define ERR_RANGE 50 //tolerance scope
static bool TP_Read_TwiceADC(uint16_t *pXCh_Adc, uint16_t *pYCh_Adc)
{
    uint16_t XCh_Adc1, YCh_Adc1, XCh_Adc2, YCh_Adc2;

    //Read the ADC values Read the ADC values twice
    TP_Read_ADC_XY(&XCh_Adc1, &YCh_Adc1);
    Driver_Delay_us(10);
    TP_Read_ADC_XY(&XCh_Adc2, &YCh_Adc2);
    Driver_Delay_us(10);

    //The ADC error used twice is greater than ERR_RANGE to take the average
    if (((XCh_Adc2 <= XCh_Adc1 && XCh_Adc1 < XCh_Adc2 + ERR_RANGE) ||
         (XCh_Adc1 <= XCh_Adc2 && XCh_Adc2 < XCh_Adc1 + ERR_RANGE)) &&
        ((YCh_Adc2 <= YCh_Adc1 && YCh_Adc1 < YCh_Adc2 + ERR_RANGE) ||
         (YCh_Adc1 <= YCh_Adc2 && YCh_Adc2 < YCh_Adc1 + ERR_RANGE)))
    {
        *pXCh_Adc = (XCh_Adc1 + XCh_Adc2) / 2;
        *pYCh_Adc = (YCh_Adc1 + YCh_Adc2) / 2;
        return true;
    }

    //The ADC error used twice is less than ERR_RANGE returns failed
    return false;
}

static uint8_t TP_Scan(uint8_t chCoordType)
{
    //In X, Y coordinate measurement, IRQ is disabled and output is low
    if (!DEV_Digital_Read(TP_IRQ_PIN))
    { //Press the button to press
        //Read the physical coordinates
        if (chCoordType)
        {
            TP_Read_TwiceADC(&sTP_DEV.Xpoint, &sTP_DEV.Ypoint);
            //Read the screen coordinates
        }
        else if (TP_Read_TwiceADC(&sTP_DEV.Xpoint, &sTP_DEV.Ypoint))
        {

            if (LCD_2_8 == id)
            {
                sTP_Draw.Xpoint = sLCD_DIS.LCD_Dis_Column -
                                  sTP_DEV.fXfac * sTP_DEV.Xpoint -
                                  sTP_DEV.iXoff;
                sTP_Draw.Ypoint = sLCD_DIS.LCD_Dis_Page -
                                  sTP_DEV.fYfac * sTP_DEV.Ypoint -
                                  sTP_DEV.iYoff;
            }
            else
            {
                //DEBUG("(Xad,Yad) = %d,%d\r\n",sTP_DEV.Xpoint,sTP_DEV.Ypoint);
                if (sTP_DEV.TP_Scan_Dir == R2L_D2U)
                { //Converts the result to screen coordinates
                    sTP_Draw.Xpoint = sTP_DEV.fXfac * sTP_DEV.Xpoint +
                                      sTP_DEV.iXoff;
                    sTP_Draw.Ypoint = sTP_DEV.fYfac * sTP_DEV.Ypoint +
                                      sTP_DEV.iYoff;
                }
                else if (sTP_DEV.TP_Scan_Dir == L2R_U2D)
                {
                    sTP_Draw.Xpoint = sLCD_DIS.LCD_Dis_Column -
                                      sTP_DEV.fXfac * sTP_DEV.Xpoint -
                                      sTP_DEV.iXoff;
                    sTP_Draw.Ypoint = sLCD_DIS.LCD_Dis_Page -
                                      sTP_DEV.fYfac * sTP_DEV.Ypoint -
                                      sTP_DEV.iYoff;
                }
                else if (sTP_DEV.TP_Scan_Dir == U2D_R2L)
                {
                    sTP_Draw.Xpoint = sTP_DEV.fXfac * sTP_DEV.Ypoint +
                                      sTP_DEV.iXoff;
                    sTP_Draw.Ypoint = sTP_DEV.fYfac * sTP_DEV.Xpoint +
                                      sTP_DEV.iYoff;
                }
                else
                {
                    sTP_Draw.Xpoint = sLCD_DIS.LCD_Dis_Column -
                                      sTP_DEV.fXfac * sTP_DEV.Ypoint -
                                      sTP_DEV.iXoff;
                    sTP_Draw.Ypoint = sLCD_DIS.LCD_Dis_Page -
                                      sTP_DEV.fYfac * sTP_DEV.Xpoint -
                                      sTP_DEV.iYoff;
                }
                // DEBUG("( x , y ) = %d,%d\r\n",sTP_Draw.Xpoint,sTP_Draw.Ypoint);
            }
        }
        if (0 == (sTP_DEV.chStatus & TP_PRESS_DOWN))
        { //Not being pressed
            sTP_DEV.chStatus = TP_PRESS_DOWN | TP_PRESSED;
            sTP_DEV.Xpoint0 = sTP_DEV.Xpoint;
            sTP_DEV.Ypoint0 = sTP_DEV.Ypoint;
        }
    }
    else
    {
        if (sTP_DEV.chStatus & TP_PRESS_DOWN)
        {                                  //0x80
            sTP_DEV.chStatus &= ~(1 << 7); //0x00
        }
        else
        {
            sTP_DEV.Xpoint0 = 0;
            sTP_DEV.Ypoint0 = 0;
            sTP_DEV.Xpoint = 0xffff;
            sTP_DEV.Ypoint = 0xffff;
        }
    }

    return (sTP_DEV.chStatus & TP_PRESS_DOWN);
}

void TP_GetAdFac(void)
{
    if (LCD_2_8 == id)
    {
        sTP_DEV.fXfac = 0.066626;
        sTP_DEV.fYfac = 0.089779;
        sTP_DEV.iXoff = -20;
        sTP_DEV.iYoff = -34;
    }
    else
    {
        if (sTP_DEV.TP_Scan_Dir == D2U_L2R)
        { //SCAN_DIR_DFT = D2U_L2R
            sTP_DEV.fXfac = -0.132443;
            sTP_DEV.fYfac = 0.089997;
            sTP_DEV.iXoff = 516;
            sTP_DEV.iYoff = -22;
        }
        else if (sTP_DEV.TP_Scan_Dir == L2R_U2D)
        {
            sTP_DEV.fXfac = 0.089697;
            sTP_DEV.fYfac = 0.134792;
            sTP_DEV.iXoff = -21;
            sTP_DEV.iYoff = -39;
        }
        else if (sTP_DEV.TP_Scan_Dir == R2L_D2U)
        {
            sTP_DEV.fXfac = 0.089915;
            sTP_DEV.fYfac = 0.133178;
            sTP_DEV.iXoff = -22;
            sTP_DEV.iYoff = -38;
        }
        else if (sTP_DEV.TP_Scan_Dir == U2D_R2L)
        {
            sTP_DEV.fXfac = -0.132906;
            sTP_DEV.fYfac = 0.087964;
            sTP_DEV.iXoff = 517;
            sTP_DEV.iYoff = -20;
        }
        else
        {
            LCD_Clear(LCD_BACKGROUND);
            GUI_DisString_EN(0, 60, "Does not support touch-screen \
							calibration in this direction",
                             &Font16, FONT_BACKGROUND, RED);
        }
    }
}


void TP_Dialog(void)
{
    LCD_Clear(LCD_BACKGROUND);

    GUI_DisString_EN(sLCD_DIS.LCD_Dis_Column - 60, 0,
                        "CLEAR", &Font16, BLACK, WHITE);
    GUI_DisString_EN(sLCD_DIS.LCD_Dis_Column - 120, 0,
                        "SAVE", &Font16, BLACK, WHITE);

    // Draw a box with black border
    GUI_DrawRectangle(BOX_X0 , BOX_Y0 ,
                      BOX_X1, BOX_Y1,
                      BLACK, DRAW_EMPTY, DOT_PIXEL_2X2);

    // NEW: also clear the shadow buffer that mirrors what's drawn
    memset(sDrawShadow, 0, sizeof(sDrawShadow));
}

void TP_Save(void)
{
    // Snapshot current drawing into the "saved" buffer
    memcpy(sSavedBitmap, sDrawShadow, sizeof(sSavedBitmap));

    // Optional on-screen / UART feedback
    GUI_DisString_EN(sLCD_DIS.LCD_Dis_Column - 120, 24,
                     "SAVED", &Font16, BLACK, WHITE);
    printf("TP_Save: captured %ux%u pixels from box [%u,%u]-[%u,%u]\r\n",
           (unsigned)BOX_W, (unsigned)BOX_H,
           (unsigned)BOX_X0, (unsigned)BOX_Y0,
           (unsigned)BOX_X1, (unsigned)BOX_Y1);

    // Print the full image as 0/1 so it "looks" like the drawing
    // (top row first, left->right)
    TP_DumpBitmapToSerial(sSavedBitmap);
}


void TP_DrawBoard(void)
{
    //	sTP_DEV.chStatus &= ~(1 << 6);
    TP_Scan(0);
    if (sTP_DEV.chStatus & TP_PRESS_DOWN)
    { 
        spi_init(SPI_PORT, 10000000);

        printf("horizontal x:%d,y:%d\n", sTP_Draw.Xpoint, sTP_Draw.Ypoint);

        if (sTP_Draw.Xpoint > (sLCD_DIS.LCD_Dis_Column - 60) &&
            sTP_Draw.Ypoint < 16)
        { 
            TP_Dialog();
        }
        else if (sTP_Draw.Xpoint > (sLCD_DIS.LCD_Dis_Column - 120) &&
                    sTP_Draw.Xpoint < (sLCD_DIS.LCD_Dis_Column - 80) &&
                    sTP_Draw.Ypoint < 24)
        { 
            TP_Save();
        }

        sTP_Draw.Color = BLACK;


        if (sTP_Draw.Xpoint > 100 && sTP_Draw.Xpoint < 380 &&
            sTP_Draw.Ypoint > 50  && sTP_Draw.Ypoint < 290)
        {
            GUI_DrawPoint(sTP_Draw.Xpoint, sTP_Draw.Ypoint,
                        sTP_Draw.Color, DOT_PIXEL_1X1, DOT_FILL_RIGHTUP);
            Capture_SetPixel(sTP_Draw.Xpoint, sTP_Draw.Ypoint);

            GUI_DrawPoint(sTP_Draw.Xpoint + 1, sTP_Draw.Ypoint,
                        sTP_Draw.Color, DOT_PIXEL_1X1, DOT_FILL_RIGHTUP);
            Capture_SetPixel(sTP_Draw.Xpoint + 1, sTP_Draw.Ypoint);

            GUI_DrawPoint(sTP_Draw.Xpoint, sTP_Draw.Ypoint + 1,
                        sTP_Draw.Color, DOT_PIXEL_1X1, DOT_FILL_RIGHTUP);
            Capture_SetPixel(sTP_Draw.Xpoint, sTP_Draw.Ypoint + 1);

            GUI_DrawPoint(sTP_Draw.Xpoint + 1, sTP_Draw.Ypoint + 1,
                        sTP_Draw.Color, DOT_PIXEL_1X1, DOT_FILL_RIGHTUP);
            Capture_SetPixel(sTP_Draw.Xpoint + 1, sTP_Draw.Ypoint + 1);

            // The DOT_PIXEL_2X2 point covers the same area; no extra capture needed
            GUI_DrawPoint(sTP_Draw.Xpoint, sTP_Draw.Ypoint,
                        sTP_Draw.Color, DOT_PIXEL_2X2, DOT_FILL_RIGHTUP);
        }

        
        spi_init(SPI_PORT, 5000000);
    }
    SPI4W_Write_Byte(0xFF);
}

void TP_Init(LCD_SCAN_DIR Lcd_ScanDir)
{
    DEV_Digital_Write(TP_CS_PIN, 1);

    sTP_DEV.TP_Scan_Dir = Lcd_ScanDir;

    TP_Read_ADC_XY(&sTP_DEV.Xpoint, &sTP_DEV.Ypoint);
}
