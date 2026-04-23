#ifndef DISCRETIZER_H
#define DISCRETIZER_H

#include "main.h"

typedef struct {
    uint8_t  resolution;
    uint16_t downsample_rate;
    uint16_t sample_counter;
    float    last_sample;
} Discretizer_t;

// Just the "blueprints" here:
void  Discretizer_Init(Discretizer_t* disc, uint8_t bits, uint16_t downsample);
float Discretizer_Process(Discretizer_t* disc, float input);

#endif
