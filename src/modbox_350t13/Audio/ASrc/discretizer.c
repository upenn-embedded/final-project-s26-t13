#include "discretizer.h"
#include <math.h>

void Discretizer_Init(Discretizer_t* disc, uint8_t bits, uint16_t downsample) {
    disc->resolution = bits;
    // Prevent division by zero or super-fast sampling
    disc->downsample_rate = (downsample < 1) ? 1 : downsample;
    disc->sample_counter = 0;
    disc->last_sample = 0.0f;
}

float Discretizer_Process(Discretizer_t* disc, float input) {
    disc->sample_counter++;

    if (disc->sample_counter >= disc->downsample_rate) {
        disc->sample_counter = 0;

        // Efficient Quantization:
        // We calculate how many "steps" the bit depth allows
        float levels = (float)(1 << disc->resolution);

        // Crush the input (assumes input is -1.0 to 1.0)
        disc->last_sample = roundf(input * levels) / levels;
    }

    return disc->last_sample;
}
