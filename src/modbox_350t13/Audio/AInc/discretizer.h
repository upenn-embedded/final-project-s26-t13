#include "discretizer.h"
#include <math.h>

typedef struct {
	uint8_t  resolution;       // Bit-depth (e.g., 8-bit or 12-bit sound)
	uint16_t downsample_rate;  // How many samples to skip (Sample-and-Hold)
	uint16_t sample_counter;   // Internal: Tracks when to grab the next sample
	float    last_sample;      // Internal: The "held" value between updates
} Discretizer_t;

void Discretizer_Init(Discretizer_t* disc, uint8_t bits, uint16_t downsample) {
    disc->resolution = bits;
    disc->downsample_rate = downsample;
    disc->sample_counter = 0;
    disc->last_sample = 0.0f;
}

float Discretizer_Process(Discretizer_t* disc, float input) {
    disc->sample_counter++;

    // Only update the value every 'downsample_rate' samples
    if (disc->sample_counter >= disc->downsample_rate) {
        disc->sample_counter = 0;

        // Quantization (Bit reduction)
        float levels = powf(2.0f, (float)disc->resolution);
        disc->last_sample = roundf(input * levels) / levels;
    }

    return disc->last_sample;
}
