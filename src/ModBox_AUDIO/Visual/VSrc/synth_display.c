#include "synth_display.h"
#include "LCD_GFX.h"
#include "ST7735.h"
#include <stdio.h>

static uint8_t last_preset = 99;
static uint8_t scroll_x = 0;

void SynthDisplay_Init(void) {
    lcd_init();
    LCD_setScreen(0x0000); // Black background
}

/**
 * @brief Main UI and Spectrogram Function
 * @param p_id: Preset ID
 * @param att, rel, fdbk: 0-255 values from UART
 * @param adc_val: 0-4095 raw ADC (for freq pos)
 * @param gain: 0.0-1.0 current envelope amplitude (for color)
 */
void SynthDisplay_Update(uint8_t p_id, uint8_t att, uint8_t rel, uint8_t fdbk, uint32_t adc_val, float gain) {
    char str[32];

    // 1. Calculate CV in mV (matching your UART logic)
    uint32_t cv_mv = (adc_val * 3300UL) / 4095UL;

    // 2. PRESET HEADER (Only redraw on change to prevent flicker)
    if (p_id != last_preset) {
        LCD_drawBlock(0, 0, 160, 18, 0x18E3); // Dark Blue Header

        // Match your pname logic - here we just use the ID
        sprintf(str, "PRESET: %d", p_id);
        LCD_drawString(5, 5, str, 0xFFFF, 0x18E3);

        // Draw static labels once
        LCD_drawString(5, 22, "CV :      mV", 0xAD55, 0x0000);
        LCD_drawString(5, 32, "ATK:", 0xFFFF, 0x0000);
        LCD_drawString(5, 42, "REL:", 0xFFFF, 0x0000);
        LCD_drawString(5, 52, "FDB:", 0xFFFF, 0x0000);

        last_preset = p_id;
    }

    // 3. UPDATE LIVE TEXT VALUES
    // We print these in a different color so they "pop"
    sprintf(str, "%4lu", cv_mv);
    LCD_drawString(35, 22, str, 0xF81F, 0x0000); // Pink for mV

    sprintf(str, "%3d", att);
    LCD_drawString(35, 32, str, 0x07E0, 0x0000); // Green for Atk

    sprintf(str, "%3d", rel);
    LCD_drawString(35, 42, str, 0x07FF, 0x0000); // Cyan for Rel

    sprintf(str, "%3d", fdbk);
    LCD_drawString(35, 52, str, 0xFBE0, 0x0000); // Orange for Fdbk

    // 4. PARAMETER BARS (Visual representation)
    // Moving them slightly to the right so they don't overlap the text
    LCD_drawBlock(65, 34, 65 + (att/3), 36, 0x07E0);
    LCD_drawBlock(65 + (att/3), 34, 155, 36, 0x0000);

    LCD_drawBlock(65, 44, 65 + (rel/3), 46, 0x07FF);
    LCD_drawBlock(65 + (rel/3), 44, 155, 46, 0x0000);

    // 5. SPECTROGRAM (Lower Half)
    uint8_t freq_pos = (uint8_t)((adc_val * 120) / 4095); // Scale for screen
    uint16_t spec_color = (gain > 0.5f) ? 0xF800 : (gain > 0.1f ? 0x07E0 : 0x001F);

    uint8_t next_col = (scroll_x + 1) % 160;
    LCD_drawBlock(next_col, 65, next_col + 1, 128, 0x0000); // Scanner gap

    // Draw the frequency dot
    LCD_drawPixel(scroll_x, 128 - (freq_pos / 2), spec_color);

    scroll_x++;
    if (scroll_x >= 160) scroll_x = 0;
}
