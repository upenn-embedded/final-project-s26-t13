/* WAVETABLE.H
 * Wavetable lookup-table (LUT) module for the modular synthesizer pipeline.
 *
 * PURPOSE
 * -------
 * Provides two things:
 *
 *   1. A set of pre-computed shape tables (exponential, sine, logarithmic)
 *      that can be used to smooth the envelope's linear ramps into natural-
 *      sounding curves.  Linear amplitude feels robotic to the ear; an
 *      exponential curve matches how human hearing perceives loudness.
 *
 *   2. A signal-processing module (Wavetable_ProcessSample) that maps an
 *      input sample through a chosen table.  Insert MOD_WAVETABLE into any
 *      preset chain to soft-map the quantised PWM steps onto a smooth curve,
 *      removing the stepped/robotic quality from the output waveform.
 *
 * USAGE IN PRESET CHAIN
 * ---------------------
 *   { .chain = { MOD_ECHO, MOD_WAVETABLE, MOD_ENVELOPE, MOD_NONE } }
 *
 * USAGE FOR ENVELOPE SHAPING
 * --------------------------
 *   Call Wavetable_ShapeEnvelope() from Pipeline_Process() after
 *   Envelope_Update() returns a linear amplitude value.  It remaps
 *   the 0.0–1.0 linear output through the exponential table so attack
 *   and release sound smooth and natural rather than robotic.
 *
 * TABLE FORMAT
 * ------------
 *   All tables are WAVETABLE_SIZE entries of float in [0.0, 1.0].
 *   Index 0 maps to input 0.0, index (SIZE-1) maps to input 1.0.
 *   Intermediate values are linearly interpolated for sub-sample accuracy.
 */

#ifndef WAVETABLE_H
#define WAVETABLE_H

#include <stdint.h>
#include <stdbool.h>

/* --------------------------------------------------------------------------
 * Table size — 256 entries uses 1 KB of flash (256 × 4 bytes).
 * Large enough for smooth interpolation; small enough to stay in cache.
 * -------------------------------------------------------------------------- */
#define WAVETABLE_SIZE  256

/* --------------------------------------------------------------------------
 * Available curve shapes
 * -------------------------------------------------------------------------- */
typedef enum {
    WT_SHAPE_EXPONENTIAL = 0,   /* x^2  — natural loudness curve, good for
                                   envelope shaping and PWM smoothing        */
    WT_SHAPE_SINE,              /* half-sine (0→π) — smooth S-curve,
                                   useful for slow sweeps and tremolo        */
    WT_SHAPE_LOGARITHMIC,       /* log  — fast initial rise, long tail,
                                   useful for percussive attack shaping      */
    WT_SHAPE_COUNT
} WavetableShape_t;

/* --------------------------------------------------------------------------
 * Module state
 * -------------------------------------------------------------------------- */
typedef struct {
    WavetableShape_t shape;     /* which table to use for ProcessSample      */
    float            mix;       /* 0.0 = dry (bypass), 1.0 = fully shaped    */
} Wavetable_t;

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

/* Initialise the module.  shape selects the active curve; mix blends between
 * the dry signal and the shaped output (0.0 = bypass, 1.0 = full effect). */
void  Wavetable_Init(Wavetable_t *wt, WavetableShape_t shape, float mix);

/* Map a normalised value (0.0–1.0) through the chosen LUT with linear
 * interpolation.  Used directly by Wavetable_ShapeEnvelope().
 * Returns a value in [0.0, 1.0]. */
float Wavetable_Lookup(WavetableShape_t shape, float t);

/* Remap a linear envelope amplitude (0.0–1.0) through the LUT so attack
 * and release curves sound natural instead of robotic.
 * Drop-in replacement: pass the return value of Envelope_Update() in,
 * use the return value of this function as the amplitude multiplier. */
float Wavetable_ShapeEnvelope(Wavetable_t *wt, float linear_amplitude);

/* Signal-path module: maps an audio sample through the LUT.
 * Input is in the pipeline's float range (approx -32768 to +32767).
 * The sample is normalised, shaped, then restored to the same range.
 * Use this as MOD_WAVETABLE in the preset chain for PWM smoothing. */
float Wavetable_ProcessSample(Wavetable_t *wt, float sample);

#endif /* WAVETABLE_H */
