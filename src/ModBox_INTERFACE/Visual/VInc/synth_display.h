#ifndef SYNTH_DISPLAY_H_
#define SYNTH_DISPLAY_H_

#include "main.h"

// Screen Dimensions for ST7735 1.8"
#define LCD_W 160
#define LCD_H 128

void SynthDisplay_Init(void);
void SynthDisplay_Update(
		uint8_t p_id,
		uint8_t att,
		uint8_t dec,
		uint8_t time,
		uint8_t flt
	);

#endif
