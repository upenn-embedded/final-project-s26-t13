/*******************************************************************************
 * ST7735.h
 * ST7735R display driver — ported from AVR to STM32F4 HAL (Nucleo-64)
 *
 * WIRING (SPI2 — avoids conflicts with TIM2/TIM3/USART1/USART2/ADC1)
 * ─────────────────────────────────────────────────────────────────────
 *  LCD pin   │ Nucleo-64 pin │ Notes
 * ───────────┼───────────────┼──────────────────────────────────────────
 *  SCK       │ PB13          │ SPI2_SCK
 *  MOSI      │ PB15          │ SPI2_MOSI
 *  CS        │ PB12          │ GPIO output (software CS)
 *  DC        │ PA8           │ GPIO output (Data/Command select)
 *  RST       │ PA9           │ GPIO output (active low reset)
 *  LITE      │ 3.3V          │ Tie high for full brightness, or use PWM later
 *  VCC       │ 3.3V          │
 *  GND       │ GND           │
 *
 * CUBEMX SETUP REQUIRED
 * ─────────────────────
 *  1. Enable SPI2: Mode = Transmit Only Master
 *                  Prescaler → target ~8 MHz (APB1=42MHz, prescaler /8 = 5.25MHz)
 *                  CPOL=Low, CPHA=1Edge, MSB first, 8-bit
 *                  NSS = Software (we drive CS manually)
 *  2. PA8, PA9, PB12 → GPIO Output, Push-Pull, No pull, High speed
 *  3. Generate code — hspi2 handle will appear in spi.h / spi.c
 ******************************************************************************/

#ifndef ST7735_H_
#define ST7735_H_

#include "main.h"       /* pulls in stm32f4xx_hal.h and all handle externs */
#include <stdint.h>
#include "spi.h"

/* --------------------------------------------------------------------------
 * External HAL handle — defined by CubeMX in spi.c
 * -------------------------------------------------------------------------- */
extern SPI_HandleTypeDef hspi2;

/* --------------------------------------------------------------------------
 * GPIO pin definitions — change here if you rewire
 * -------------------------------------------------------------------------- */
#define LCD_CS_PORT     GPIOB
#define LCD_CS_PIN      GPIO_PIN_10

#define LCD_DC_PORT     GPIOB
#define LCD_DC_PIN      GPIO_PIN_1

#define LCD_RST_PORT    GPIOB
#define LCD_RST_PIN     GPIO_PIN_2



/* --------------------------------------------------------------------------
 * Convenience macros — replace AVR set()/clear() with HAL calls
 * -------------------------------------------------------------------------- */
#define LCD_CS_LOW()    HAL_GPIO_WritePin(LCD_CS_PORT,  LCD_CS_PIN,  GPIO_PIN_RESET)
#define LCD_CS_HIGH()   HAL_GPIO_WritePin(LCD_CS_PORT,  LCD_CS_PIN,  GPIO_PIN_SET)
#define LCD_DC_LOW()    HAL_GPIO_WritePin(LCD_DC_PORT,  LCD_DC_PIN,  GPIO_PIN_RESET)
#define LCD_DC_HIGH()   HAL_GPIO_WritePin(LCD_DC_PORT,  LCD_DC_PIN,  GPIO_PIN_SET)
#define LCD_RST_LOW()   HAL_GPIO_WritePin(LCD_RST_PORT, LCD_RST_PIN, GPIO_PIN_RESET)
#define LCD_RST_HIGH()  HAL_GPIO_WritePin(LCD_RST_PORT, LCD_RST_PIN, GPIO_PIN_SET)

/* --------------------------------------------------------------------------
 * Display dimensions
 * -------------------------------------------------------------------------- */
#define LCD_WIDTH   160
#define LCD_HEIGHT  128
#define LCD_SIZE    (LCD_WIDTH * LCD_HEIGHT)

/* --------------------------------------------------------------------------
 * ST7735 command registers (unchanged from AVR version)
 * -------------------------------------------------------------------------- */
#define ST7735_NOP      0x00
#define ST7735_SWRESET  0x01
#define ST7735_RDDID    0x04
#define ST7735_RDDST    0x09
#define ST7735_SLPIN    0x10
#define ST7735_SLPOUT   0x11
#define ST7735_PTLON    0x12
#define ST7735_NORON    0x13
#define ST7735_INVOFF   0x20
#define ST7735_INVON    0x21
#define ST7735_DISPOFF  0x28
#define ST7735_DISPON   0x29
#define ST7735_CASET    0x2A
#define ST7735_RASET    0x2B
#define ST7735_RAMWR    0x2C
#define ST7735_RAMRD    0x2E
#define ST7735_PTLAR    0x30
#define ST7735_COLMOD   0x3A
#define ST7735_MADCTL   0x36
#define ST7735_FRMCTR1  0xB1
#define ST7735_FRMCTR2  0xB2
#define ST7735_FRMCTR3  0xB3
#define ST7735_INVCTR   0xB4
#define ST7735_DISSET5  0xB6
#define ST7735_PWCTR1   0xC0
#define ST7735_PWCTR2   0xC1
#define ST7735_PWCTR3   0xC2
#define ST7735_PWCTR4   0xC3
#define ST7735_PWCTR5   0xC4
#define ST7735_VMCTR1   0xC5
#define ST7735_RDID1    0xDA
#define ST7735_RDID2    0xDB
#define ST7735_RDID3    0xDC
#define ST7735_RDID4    0xDD
#define ST7735_PWCTR6   0xFC
#define ST7735_GMCTRP1  0xE0
#define ST7735_GMCTRN1  0xE1

/* MADCTL orientation bits */
#define MADCTL_MY   0x80
#define MADCTL_MX   0x40
#define MADCTL_MV   0x20
#define MADCTL_ML   0x10
#define MADCTL_RGB  0x00
#define MADCTL_BGR  0x08
#define MADCTL_MH   0x04

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */
void lcd_init(void);
void sendCommands(const uint8_t *cmds, uint8_t length);
void LCD_setAddr(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);

void SPI_ControllerTx(uint8_t data);
void SPI_ControllerTx_stream(uint8_t stream);
void SPI_ControllerTx_16bit(uint16_t data);
void SPI_ControllerTx_16bit_stream(uint16_t data);

void LCD_brightness(uint8_t intensity);   /* stub — tie LITE to 3.3V for now */
void LCD_rotate(uint8_t r);
void Delay_ms(unsigned int n);

#endif /* ST7735_H_ */
