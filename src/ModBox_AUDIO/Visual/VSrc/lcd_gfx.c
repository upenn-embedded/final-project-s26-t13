/*******************************************************************************
 * LCD_GFX.c
 * Graphics layer for ST7735R — ported from AVR to STM32F4 HAL
 *
 * PORTING NOTES
 * ─────────────
 * All drawing logic is identical to the AVR version.
 * The only changes are:
 *   - Removed #include <avr/io.h>
 *   - LCD_drawBlock: replaced set()/clear() bit-bang macros with
 *     LCD_CS_HIGH() / LCD_CS_LOW() from ST7735.h
 *   - Everything else compiles as-is against the HAL SPI backend
 ******************************************************************************/

#include "lcd_gfx.h"
#include "ST7735.h"
#include <stdlib.h>

/* --------------------------------------------------------------------------
 * rgb565 — convert 8-bit R/G/B to packed 16-bit RGB565
 * -------------------------------------------------------------------------- */
uint16_t rgb565(uint8_t red, uint8_t green, uint8_t blue)
{
    return (uint16_t)(
        ( ((31  * (red   + 4)) / 255) << 11 ) |
        ( ((63  * (green + 2)) / 255) <<  5 ) |
        ( ((31  * (blue  + 4)) / 255)        )
    );
}

/* --------------------------------------------------------------------------
 * LCD_drawPixel — single pixel
 * -------------------------------------------------------------------------- */
void LCD_drawPixel(uint8_t x, uint8_t y, uint16_t color)
{
    LCD_setAddr(x, y, x, y);
    SPI_ControllerTx_16bit(color);
}

/* --------------------------------------------------------------------------
 * LCD_drawChar — 5×8 character from ASCII_LUT
 * -------------------------------------------------------------------------- */
void LCD_drawChar(uint8_t x, uint8_t y, uint16_t character,
                  uint16_t fColor, uint16_t bColor)
{
    uint16_t row = character - 0x20;    /* ASCII table starts at space (0x20) */
    if ((LCD_WIDTH - x > 7) && (LCD_HEIGHT - y > 7))
    {
        for (int i = 0; i < 5; i++)
        {
            uint8_t pixels = ASCII[row][i];
            for (int j = 0; j < 8; j++)
            {
                if ((pixels >> j) & 1)
                    LCD_drawPixel(x + i, y + j, fColor);
                else
                    LCD_drawPixel(x + i, y + j, bColor);
            }
        }
    }
}

/* --------------------------------------------------------------------------
 * LCD_drawCircle — filled circle via horizontal spans
 * -------------------------------------------------------------------------- */
void LCD_drawCircle(uint8_t x0, uint8_t y0, uint8_t radius, uint16_t color)
{
    int8_t x = (int8_t)radius;
    for (int8_t y = 0; y <= (int8_t)radius; y++)
    {
        while ((int16_t)x * x + (int16_t)y * y > (int16_t)radius * radius)
            x--;
        LCD_drawBlock((uint8_t)(x0 - x), (uint8_t)(y0 + y),
                      (uint8_t)(x0 + x), (uint8_t)(y0 + y), color);
        if (y != 0)
            LCD_drawBlock((uint8_t)(x0 - x), (uint8_t)(y0 - y),
                          (uint8_t)(x0 + x), (uint8_t)(y0 - y), color);
    }
}

/* --------------------------------------------------------------------------
 * LCD_drawLine — Bresenham's line algorithm
 * -------------------------------------------------------------------------- */
void LCD_drawLine(short x0, short y0, short x1, short y1, uint16_t c)
{
    short dx =  (short)abs(x1 - x0);
    short sx = (x0 < x1) ? 1 : -1;
    short dy = -(short)abs(y1 - y0);
    short sy = (y0 < y1) ? 1 : -1;
    short err = dx + dy;

    while (1)
    {
        LCD_drawPixel((uint8_t)x0, (uint8_t)y0, c);
        if (x0 == x1 && y0 == y1) break;
        short e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

/* --------------------------------------------------------------------------
 * LCD_drawBlock — filled rectangle (bulk pixel fill)
 *
 * AVR version used set(LCD_PORT, LCD_TFT_CS) / clear(LCD_PORT, LCD_TFT_CS)
 * which are AVR register bit-manipulation macros.
 * Replaced with LCD_CS_HIGH() / LCD_CS_LOW() from ST7735.h — same behaviour.
 * -------------------------------------------------------------------------- */
void LCD_drawBlock(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1,
                   uint16_t color)
{
    LCD_setAddr(x0, y0, x1, y1);
    uint32_t total = (uint32_t)(x1 - x0 + 1) * (uint32_t)(y1 - y0 + 1);

    LCD_CS_LOW();
    for (uint32_t i = 0; i < total; i++)
        SPI_ControllerTx_16bit_stream(color);
    LCD_CS_HIGH();
}

/* --------------------------------------------------------------------------
 * LCD_setScreen — fill entire display with one color
 * -------------------------------------------------------------------------- */
void LCD_setScreen(uint16_t color)
{
    LCD_drawBlock(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1, color);
}

/* --------------------------------------------------------------------------
 * LCD_drawString — draw a null-terminated string
 * -------------------------------------------------------------------------- */
void LCD_drawString(uint8_t x, uint8_t y, char *str, uint16_t fg, uint16_t bg)
{
    uint8_t cursor_x = x;
    while (*str)
    {
        LCD_drawChar(cursor_x, y, (uint16_t)*str, fg, bg);
        cursor_x += 6;      /* 5px glyph + 1px spacing */
        str++;
    }
}
