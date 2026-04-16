#include "discretizer.h"
#include <math.h>

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
