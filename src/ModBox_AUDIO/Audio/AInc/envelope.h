#ifndef ENVELOPE_H
#define ENVELOPE_H

#include "main.h"

typedef enum {
    IDLE,
    ATTACK,
    DECAY,
    SUSTAIN,
    RELEASE
} ENVELOPE_STATE;

typedef struct {
    float attack_rate;  // Increment per sample
    float decay_rate;   // Decrement per sample
    float sustain_level; // 0.0 to 1.0
    float release_rate;  // Decrement per sample
    float current_amplitude;
    ENVELOPE_STATE state;
} ADSR_t;

// Function Prototypes
void Envelope_Init(ADSR_t* adsr);
void Envelope_Trigger(ADSR_t* adsr);
void Envelope_Release(ADSR_t* adsr);
float Envelope_Update(ADSR_t* adsr);

#endif
