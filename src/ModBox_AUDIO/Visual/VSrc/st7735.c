/*******************************************************************************
 * ST7735.c
 * ST7735R display driver — ported from AVR to STM32F4 HAL (Nucleo-64)
 *
 * PORTING NOTES
 * ─────────────
 * AVR                          │ STM32 replacement
 * ─────────────────────────────┼──────────────────────────────────────────────
 * SPCR0/SPDR0/SPSR0 registers  │ HAL_SPI_Transmit(&hspi2, ...)
 * DDRB / PORTB direction setup │ CubeMX GPIO init (not in code)
 * set(LCD_PORT, LCD_TFT_CS)    │ LCD_CS_HIGH() macro → HAL_GPIO_WritePin()
 * clear(LCD_PORT, LCD_DC)      │ LCD_DC_LOW()  macro → HAL_GPIO_WritePin()
 * _delay_ms(n)                 │ HAL_Delay(n)
 * TCCR0A / OCR0A (backlight)   │ LITE tied to 3.3V; LCD_brightness() is stub
 * avr/io.h, avr/pgmspace.h     │ removed
 ******************************************************************************/

#include "ST7735.h"

/* --------------------------------------------------------------------------
 * Delay wrapper
 * -------------------------------------------------------------------------- */
void Delay_ms(unsigned int n)
{
    HAL_Delay(n);
}

/* --------------------------------------------------------------------------
 * SPI transmit helpers
 *
 * HAL_SPI_Transmit() blocks until the byte is shifted out, which is the
 * same behaviour as polling SPIF on AVR.  Timeout = 10 ms is generous for
 * a single byte at ~5 MHz.
 * -------------------------------------------------------------------------- */

/* Send one byte, toggling CS around it */
void SPI_ControllerTx(uint8_t data)
{
    LCD_CS_LOW();
    HAL_SPI_Transmit(&hspi2, &data, 1, 10);
    LCD_CS_HIGH();
}

/* Send one byte without touching CS — used inside sendCommands() bulk transfers */
void SPI_ControllerTx_stream(uint8_t stream)
{
    HAL_SPI_Transmit(&hspi2, &stream, 1, 10);
}

/* Send 16-bit colour word, toggling CS */
void SPI_ControllerTx_16bit(uint16_t data)
{
    uint8_t buf[2] = { (uint8_t)(data >> 8), (uint8_t)(data & 0xFF) };
    LCD_CS_LOW();
    HAL_SPI_Transmit(&hspi2, buf, 2, 10);
    LCD_CS_HIGH();
}

/* Send 16-bit colour word without touching CS — used inside pixel fill loops */
void SPI_ControllerTx_16bit_stream(uint16_t data)
{
    uint8_t buf[2] = { (uint8_t)(data >> 8), (uint8_t)(data & 0xFF) };
    HAL_SPI_Transmit(&hspi2, buf, 2, 10);
}

/* --------------------------------------------------------------------------
 * sendCommands
 *
 * Parses the packed command array format:
 *   [ CMD, num_data_bytes, data..., delay_ms ]
 * identical to the AVR version — only the SPI and delay calls differ.
 * -------------------------------------------------------------------------- */
void sendCommands(const uint8_t *cmds, uint8_t length)
{
    uint8_t numCommands = length;

    LCD_CS_LOW();

    while (numCommands--)
    {
        /* Send command byte with DC low */
        LCD_DC_LOW();
        SPI_ControllerTx_stream(*cmds++);

        uint8_t numData = *cmds++;      /* number of following data bytes */

        LCD_DC_HIGH();
        while (numData--)
            SPI_ControllerTx_stream(*cmds++);

        uint8_t waitTime = *cmds++;     /* post-command delay in ms */
        if (waitTime != 0)
            Delay_ms((waitTime == 255) ? 500 : waitTime);
    }

    LCD_CS_HIGH();
}

/* --------------------------------------------------------------------------
 * lcd_init
 *
 * Hardware reset then the ST7735 init command sequence.
 * AVR did hardware reset via lcd_pin_init() + _delay_ms.
 * STM32: GPIO directions set by CubeMX; we just drive the RST line here.
 * -------------------------------------------------------------------------- */
void lcd_init(void)
{
    /* Hardware reset pulse */
    LCD_RST_LOW();
    Delay_ms(50);
    LCD_RST_HIGH();
    Delay_ms(5);

    static const uint8_t ST7735_cmds[] =
    {
        ST7735_SWRESET, 0, 150,
        ST7735_SLPOUT,  0, 255,
        ST7735_FRMCTR1, 3, 0x01, 0x2C, 0x2D, 0,
        ST7735_FRMCTR2, 3, 0x01, 0x2C, 0x2D, 0,
        ST7735_FRMCTR3, 6, 0x01, 0x2C, 0x2D, 0x01, 0x2C, 0x2D, 0,
        ST7735_INVCTR,  1, 0x07, 0,
        ST7735_PWCTR1,  3, 0x0A, 0x02, 0x84, 5,
        ST7735_PWCTR2,  1, 0xC5, 5,
        ST7735_PWCTR3,  2, 0x0A, 0x00, 5,
        ST7735_PWCTR4,  2, 0x8A, 0x2A, 5,
        ST7735_PWCTR5,  2, 0x8A, 0xEE, 5,
        ST7735_VMCTR1,  1, 0x0E, 0,
        ST7735_INVOFF,  0, 0,
        ST7735_MADCTL,  1, 0xC8, 0,
        ST7735_COLMOD,  1, 0x05, 0,
        ST7735_CASET,   4, 0x00, 0x00, 0x00, 0x7F, 0,
        ST7735_RASET,   4, 0x00, 0x00, 0x00, 0x9F, 0,
        ST7735_GMCTRP1, 16, 0x02, 0x1C, 0x07, 0x12, 0x37, 0x32, 0x29, 0x2D,
                            0x29, 0x25, 0x2B, 0x39, 0x00, 0x01, 0x03, 0x10, 0,
        ST7735_GMCTRN1, 16, 0x03, 0x1D, 0x07, 0x06, 0x2E, 0x2C, 0x29, 0x2D,
                            0x2E, 0x2E, 0x37, 0x3F, 0x00, 0x00, 0x02, 0x10, 0,
        ST7735_NORON,   0, 10,
        ST7735_DISPON,  0, 100,
        ST7735_MADCTL,  1, MADCTL_MX | MADCTL_MV | MADCTL_RGB, 10
    };

    sendCommands(ST7735_cmds, 22);
}

/* --------------------------------------------------------------------------
 * LCD_setAddr — set pixel write window
 * -------------------------------------------------------------------------- */
void LCD_setAddr(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    const uint8_t cmds[] =
    {
        ST7735_CASET, 4, 0x00, x0, 0x00, x1, 0,
        ST7735_RASET, 4, 0x00, y0, 0x00, y1, 0,
        ST7735_RAMWR, 0, 5
    };
    sendCommands(cmds, 3);
}

/* --------------------------------------------------------------------------
 * LCD_brightness — stub (LITE tied to 3.3V)
 * If you later wire LITE to a PWM-capable pin, drive it here with
 * __HAL_TIM_SET_COMPARE().
 * -------------------------------------------------------------------------- */
void LCD_brightness(uint8_t intensity)
{
    (void)intensity;    /* not implemented — tie LITE to 3.3V */
}

/* --------------------------------------------------------------------------
 * LCD_rotate
 * -------------------------------------------------------------------------- */
void LCD_rotate(uint8_t r)
{
    uint8_t madctl = 0;
    switch (r % 4)
    {
        case 0: madctl = MADCTL_MX | MADCTL_MY | MADCTL_RGB; break;
        case 1: madctl = MADCTL_MY | MADCTL_MV | MADCTL_RGB; break;
        case 2: madctl = MADCTL_RGB;                          break;
        case 3: madctl = MADCTL_MX | MADCTL_MV | MADCTL_RGB; break;
        default:madctl = MADCTL_MX | MADCTL_MY | MADCTL_RGB; break;
    }
    const uint8_t cmds[] = { ST7735_MADCTL, 1, madctl, 0 };
    sendCommands(cmds, 1);
}
