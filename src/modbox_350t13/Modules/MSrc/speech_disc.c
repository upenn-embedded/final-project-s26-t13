/* speech_discretizer.c
 * Speech Discretizer Module — ModBox
 *
 * Recording phase:
 *   Each call to Discretizer_Process() while RECORDING stores the incoming
 *   mic sample into the buffer and advances record_idx. When the buffer is
 *   full, recording stops automatically and playback begins immediately.
 *
 * Playback phase:
 *   play_idx is a float that advances by play_speed each sample.
 *   Linear interpolation between adjacent buffer samples gives smooth
 *   pitch-shifted output without harsh zipper noise.
 *   When play_idx reaches record_len, it wraps to 0 (seamless loop).
 *
 * Pitch shifting via speed:
 *   play_speed = 1.0 → original pitch
 *   play_speed = 2.0 → one octave up (reads buffer twice as fast)
 *   play_speed = 0.5 → one octave down (reads buffer half as fast)
 *   Knob center (128) = 1.0x speed, linear mapping each side.
 */

#include "speech_discretizer.h"
#include <string.h>

void Discretizer_Init(Discretizer_t *disc) {
    memset(disc->buffer, 0, sizeof(disc->buffer));
    disc->record_len = 0;
    disc->record_idx = 0;
    disc->play_idx   = 0.0f;
    disc->play_speed = 1.0f;
    disc->state      = DISC_IDLE;
    disc->bypassed   = false;
}

void Discretizer_StartRecord(Discretizer_t *disc) {
    memset(disc->buffer, 0, sizeof(disc->buffer));
    disc->record_idx = 0;
    disc->record_len = 0;
    disc->state      = DISC_RECORDING;
}

void Discretizer_StartPlay(Discretizer_t *disc) {
    if (disc->record_len == 0) return;   /* nothing captured yet */
    disc->play_idx = 0.0f;
    disc->state    = DISC_PLAYING;
}

void Discretizer_Stop(Discretizer_t *disc) {
    disc->state = DISC_IDLE;
}

int16_t Discretizer_Process(Discretizer_t *disc, int16_t mic_sample) {
    if (disc->bypassed) return mic_sample;   /* pass mic through unmodified */

    switch (disc->state) {

        case DISC_IDLE:
            return 0;

        case DISC_RECORDING:
            disc->buffer[disc->record_idx] = mic_sample;
            disc->record_idx++;

            if (disc->record_idx >= DISC_BUFFER_SAMPLES) {
                /* Buffer full — auto-switch to playback */
                disc->record_len = DISC_BUFFER_SAMPLES;
                disc->play_idx   = 0.0f;
                disc->state      = DISC_PLAYING;
            }
            return 0;   /* no output during recording */

        case DISC_PLAYING: {
            if (disc->record_len == 0) return 0;

            /* Linear interpolation between two adjacent buffer samples */
            uint32_t idx0 = (uint32_t)disc->play_idx;
            uint32_t idx1 = (idx0 + 1) % disc->record_len;
            float    frac = disc->play_idx - (float)idx0;

            float s0 = (float)disc->buffer[idx0];
            float s1 = (float)disc->buffer[idx1];
            float interpolated = s0 + frac * (s1 - s0);

            /* Advance playback head and wrap */
            disc->play_idx += disc->play_speed;
            if (disc->play_idx >= (float)disc->record_len) {
                disc->play_idx -= (float)disc->record_len;
            }

            /* Clamp to int16 range */
            if (interpolated >  32767.0f) interpolated =  32767.0f;
            if (interpolated < -32767.0f) interpolated = -32767.0f;

            return (int16_t)interpolated;
        }

        default:
            return 0;
    }
}

void Discretizer_SetParam(Discretizer_t *disc, uint8_t knob_value) {
    /* Map knob 0–255 to play_speed 0.5–2.0
     * Knob 128 (center) = 1.0 (original pitch)
     * Knob 0   = 0.5  (one octave down)
     * Knob 255 = 2.0  (one octave up)
     *
     * Two linear segments meeting at center: */
    float t = (float)knob_value / 255.0f;   /* 0.0 – 1.0 */
    disc->play_speed = 0.5f + t * 1.5f;     /* 0.5 – 2.0 */
}

void Discretizer_SetBypassed(Discretizer_t *disc, bool bypassed) {
    disc->bypassed = bypassed;
    if (bypassed) disc->state = DISC_IDLE;
}

DiscretizerState_t Discretizer_GetState(const Discretizer_t *disc) {
    return disc->state;
}
