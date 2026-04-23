#include "mic.h"
#include "adc.h"
#include "main.h"
#include "usart.h"
#include <string.h>
#include <stdbool.h>

// MIC READS INTO PIN PA1 ON THE AUDIO STM

// 1. DEFINE CONSTANTS AT THE TOP
#define MIC_OFFSET 2048.0f
#define NOISE_THRESHOLD 0.15f
#define DURATION_SAMPLES 800

// 2. DEFINE STATIC VARIABLES AT THE TOP
static uint32_t noise_counter = 0;
static bool message_sent = false;

static float filter_state = 0.0f;
static const float alpha = 0.6f;

float Mic_ReadSample(void) {
    uint32_t raw_val = 0;

    // ADC trigger
    HAL_ADC_Start(&hadc1);

    // Conversion with polling (v fast)
    if (HAL_ADC_PollForConversion(&hadc1, 1) == HAL_OK) {
        raw_val = HAL_ADC_GetValue(&hadc1);
    }

    // convert to -1.0 to 1.0 range
    float raw_float = ((float)raw_val - MIC_OFFSET) / MIC_OFFSET;

	filter_state = (alpha * raw_float) + ((1.0f - alpha) * filter_state);

	// NOISE DETECTION W UART
	float amplitude = (filter_state < 0) ? -filter_state : filter_state;

	if (amplitude > NOISE_THRESHOLD) {
		noise_counter++;

		if (noise_counter >= DURATION_SAMPLES && !message_sent) {
			char alert[] = "[ALERT] Noise Detected!\r\n";
			HAL_UART_Transmit(&huart2, (uint8_t*)alert, strlen(alert), 10);
			message_sent = true;
		}
	} else {
		// Reset if it goes quiet (or use a small "grace period" if you prefer)
		noise_counter = 0;
		message_sent = false;
	}

	// Bound the output
	if (filter_state > 1.0f)  filter_state = 1.0f;
	if (filter_state < -1.0f) filter_state = -1.0f;

	return filter_state;
}
