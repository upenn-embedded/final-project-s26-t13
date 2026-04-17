/* speech_discretizer.h
 * Speech Discretizer Module — ModBox
 *
 * Records a window of microphone input, then loops it periodically as a
 * pitched waveform. This converts a live voice signal into something the
 * analog module chain (VCF, VCA, echo) can treat like an oscillator output.
 *
 * States:
 *   IDLE      — not recording, not playing, output is silence
 *   RECORDING — filling the capture buffer from mic ADC input
 *   PLAYING   — looping the captured buffer as output
 *
 * The record/play toggle is triggered by the button on PC13 (MCU A side)
 * or commanded over I2C from MCU I. Call Discretizer_StartRecord() and
 * Discretizer_StartPlay() from your button/I2C handler.
 *
 * Playback speed (and therefore pitch) is controlled by the knob —
 * knob mid = original pitch, knob up = higher pitch (faster playback),
 * knob down = lower pitch (slower playback via linear interpolation).
 *
 * Buffer: DISC_BUFFER_SAMPLES at your sample rate defines max record length.
 * At 8kHz, 8000 samples = 1 second. Tune to available RAM.
 */

#ifndef SPEECH_DISCRETIZER_H
#define SPEECH_DISCRETIZER_H

#include <stdint.h>
#include <stdbool.h>

#define DISC_BUFFER_SAMPLES  8000   /* max record length in samples */

typedef enum {
    DISC_IDLE      = 0,
    DISC_RECORDING = 1,
    DISC_PLAYING   = 2,
} DiscretizerState_t;

typedef struct {
    int16_t            buffer[DISC_BUFFER_SAMPLES];
    uint32_t           record_len;      /* how many samples were captured */
    uint32_t           record_idx;      /* write head during recording */
    float              play_idx;        /* fractional read head during playback */
    float              play_speed;      /* 0.5=half pitch, 1.0=normal, 2.0=octave up */
    DiscretizerState_t state;
    bool               bypassed;
} Discretizer_t;

void    Discretizer_Init(Discretizer_t *disc);

/* Call from button ISR or I2C command handler */
void    Discretizer_StartRecord(Discretizer_t *disc);
void    Discretizer_StartPlay(Discretizer_t *disc);
void    Discretizer_Stop(Discretizer_t *disc);

/* Feed mic sample in. Returns:
 *   IDLE/RECORDING: 0 (silence or recording, no output yet)
 *   PLAYING:        next interpolated playback sample */
int16_t Discretizer_Process(Discretizer_t *disc, int16_t mic_sample);

/* Knob 0–255: 128 = original pitch, 0 = half speed, 255 = double speed */
void    Discretizer_SetParam(Discretizer_t *disc, uint8_t knob_value);

void    Discretizer_SetBypassed(Discretizer_t *disc, bool bypassed);

/* Returns current state — useful for UART debug output */
DiscretizerState_t Discretizer_GetState(const Discretizer_t *disc);

#endif /* SPEECH_DISCRETIZER_H */
