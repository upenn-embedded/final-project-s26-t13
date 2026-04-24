#include "synth_display.h"
#include "lcd_gfx.h"
#include "ST7735.h"
#include <stdio.h>
#include "spi.h"

static uint8_t last_preset = 99;

void SynthDisplay_Init(void) {
//	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);
//	HAL_Delay(50);
//	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_SET);
//	HAL_Delay(50);
    lcd_init();
    LCD_setScreen(BLACK); // Black background
}

/**
 * @brief Main UI and Spectrogram Function
 * @param p_id: Preset ID
 * @param att, rel, fdbk: 0-255 values from UART
 * @param adc_val: 0-4095 raw ADC (for freq pos)
 * @param gain: 0.0-1.0 current envelope amplitude (for color)
 */
void SynthDisplay_Update(uint8_t p_id, uint8_t att, uint8_t dec, uint8_t time, uint8_t flt) {
    char str[32];

    if (p_id != last_preset) {
        LCD_drawBlock(0, 0, 160, 18, 0x18E3);
        sprintf(str, "PRESET: %d", p_id);
        LCD_drawString(5, 5, str, 0xFFFF, 0x18E3);

        /* Static labels — drawn once per preset change */
        LCD_drawString(5, 22, "ATK:", 0xFFFF, 0x0000);
        LCD_drawString(5, 32, "DEC:", 0xFFFF, 0x0000);
        LCD_drawString(5, 42, "TIME:", 0xFFFF, 0x0000);
        LCD_drawString(5, 52, "FLT:", 0xFFFF, 0x0000);

        last_preset = p_id;
    }

    /* Live values — each value at the same y as its label */
    sprintf(str, "%3d", att);
    LCD_drawString(35, 22, str, 0x07E0, 0x0000);   /* green  — ATK */

    sprintf(str, "%3d", dec);
    LCD_drawString(35, 32, str, 0x07FF, 0x0000);   /* cyan   — DEC */

    sprintf(str, "%3d", time);
    LCD_drawString(35, 42, str, 0xFBE0, 0x0000);   /* orange — TIME */

    sprintf(str, "%3d", flt);
    LCD_drawString(35, 52, str, 0xF81F, 0x0000);   /* pink   — FLT */

    /* Parameter bars */
    LCD_drawBlock(65, 24, 65 + (att/3),  26, 0x07E0);
    LCD_drawBlock(65 + (att/3),  24, 155, 26, 0x0000);

    LCD_drawBlock(65, 34, 65 + (dec/3),  36, 0x07FF);
    LCD_drawBlock(65 + (dec/3),  34, 155, 36, 0x0000);

    LCD_drawBlock(65, 44, 65 + (time/3), 46, 0xFBE0);
    LCD_drawBlock(65 + (time/3), 44, 155, 46, 0x0000);

    LCD_drawBlock(65, 54, 65 + (flt/3),  56, 0xF81F);
    LCD_drawBlock(65 + (flt/3),  54, 155, 56, 0x0000);
}
