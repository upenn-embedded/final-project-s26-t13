#ifndef ECHO_H
#define ECHO_H

#include "main.h"

#define ECHO_BUFFER_SIZE 16000 // Approx 2 seconds at 8kHz

typedef struct {
    float buffer[ECHO_BUFFER_SIZE];
    uint32_t write_pos;
    float feedback;  // 0.0 to 0.9 (don't go to 1.0 or it will loop forever/clip)
    float mix;       // 0.0 (dry) to 1.0 (wet)
    uint32_t delay_samples;
} Echo_t;

void Echo_Init(Echo_t* echo, uint32_t delay_ms);
float Echo_Process(Echo_t* echo, float input);

#endif
