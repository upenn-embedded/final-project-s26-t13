/* envelope.h
 * Envelope Follower Module
 *
 * control value from 0-4095 produced based on amplitude envelope of incoming audio
 * Tracks the amplitude envelope of an incoming audio signal and produces
 * a smoothed control value (0–4095) proportional to signal loudness.
 * Used internally by the speech discretizer and VCA control path.
 *
 * Attack: how fast the envelope rises when signal gets louder
 * Release: how fast the envelope falls when signal gets quieter
 * Both are set as coefficients 0.0–1.0 (higher = faster response)
 */

#ifndef ENVELOPE_H
#define ENVELOPE_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    float    level;            /* current internal level (0.0 to 1.0) */
    float    attack_coeff;
    float    release_coeff;
    uint16_t output;           /* 0–4095 for your DAC/VCA CV */
    bool     bypassed;
} EnvelopeState_t;

void     Envelope_Init(EnvelopeState_t *env);
/* Replaced 'sample' with 'gate' */
uint16_t Envelope_Process(EnvelopeState_t *env, bool gate);
void     Envelope_SetParam(EnvelopeState_t *env, uint8_t knob_value);
void     Envelope_SetBypassed(EnvelopeState_t *env, bool bypassed);

#endif
