/* WAVETABLE.C
 * Wavetable lookup-table (LUT) module for the modular synthesizer pipeline.
 *
 * HOW THE TABLES ARE BUILT
 * ------------------------
 * All three tables are computed once at startup by Wavetable_Init() the
 * first time it is called (lazy initialisation via a static flag).
 * Using float arithmetic at init-time is fine — it runs once and the
 * results are cached in the static arrays for the lifetime of the program.
 *
 * WHY NOT CONST ARRAYS IN FLASH?
 * -------------------------------
 * Pre-computed const float arrays would need to be hand-calculated and
 * hard-coded, making them hard to audit or adjust.  The lazy-init approach
 * lets the math speak for itself and costs nothing at audio-loop time.
 * If flash space is tight, convert to const arrays after verifying the
 * values with a unit test.
 *
 * INTERPOLATION
 * -------------
 * Wavetable_Lookup() uses linear interpolation between adjacent table
 * entries.  At 256 entries this gives better than 0.4% accuracy across
 * the full range — sufficient for audio envelope shaping.
 */

#include "wavetable.h"
#include <stdbool.h>    /* bool, true, false */
#include <math.h>       /* sinf(), log10f() — link with -lm */

/* --------------------------------------------------------------------------
 * Internal LUT storage — one array per shape, shared across all instances.
 * 256 floats × 3 tables × 4 bytes = 3 KB of RAM.
 * -------------------------------------------------------------------------- */
static float    lut[WT_SHAPE_COUNT][WAVETABLE_SIZE];
static bool     lut_ready = false;

/* --------------------------------------------------------------------------
 * lut_build  (private)
 *
 * Fills all three tables once.  Called automatically on first Wavetable_Init.
 *
 * Table definitions:
 *
 *   EXPONENTIAL  f(t) = t²
 *     Maps linear 0→1 onto a slow-start, fast-finish curve.
 *     Matches the ear's logarithmic loudness perception — a linear amplitude
 *     ramp sounds like it accelerates; an exponential ramp sounds even.
 *     Best choice for envelope shaping and general PWM smoothing.
 *
 *   SINE         f(t) = sin(t × π/2)
 *     Maps linear 0→1 onto a quarter-sine curve (smooth S shape).
 *     Very gentle start and end, useful for slow filter sweeps or tremolo.
 *     Avoids the "snap" of exponential at t=1.
 *
 *   LOGARITHMIC  f(t) = log(1 + t×9) / log(10)
 *     Fast initial rise that flattens toward 1.0.
 *     Useful for percussive attack shaping where the transient needs to
 *     pop quickly and the tail needs to sustain.
 * -------------------------------------------------------------------------- */
static void lut_build(void)
{
    for (int i = 0; i < WAVETABLE_SIZE; i++)
    {
        float t = (float)i / (float)(WAVETABLE_SIZE - 1);  /* 0.0 … 1.0 */

        /* Exponential: t² */
        lut[WT_SHAPE_EXPONENTIAL][i] = t * t;

        /* Sine: quarter-sine, 0 → π/2 */
        lut[WT_SHAPE_SINE][i] = sinf(t * (3.14159265f / 2.0f));

        /* Logarithmic: log₁₀(1 + 9t), normalised to [0,1] */
        lut[WT_SHAPE_LOGARITHMIC][i] = log10f(1.0f + t * 9.0f);
    }

    lut_ready = true;
}

/* --------------------------------------------------------------------------
 * Wavetable_Init
 * -------------------------------------------------------------------------- */
void Wavetable_Init(Wavetable_t *wt, WavetableShape_t shape, float mix)
{
    if (!lut_ready)
        lut_build();

    wt->shape = (shape < WT_SHAPE_COUNT) ? shape : WT_SHAPE_EXPONENTIAL;
    wt->mix   = (mix   < 0.0f) ? 0.0f : (mix > 1.0f) ? 1.0f : mix;
}

/* --------------------------------------------------------------------------
 * Wavetable_Lookup
 *
 * Maps a normalised input t ∈ [0.0, 1.0] through the chosen LUT using
 * linear interpolation between adjacent entries.
 *
 * Linear interpolation formula:
 *   output = lut[i] + frac × (lut[i+1] - lut[i])
 * where i = floor(t × (SIZE-1)) and frac is the fractional remainder.
 * -------------------------------------------------------------------------- */
float Wavetable_Lookup(WavetableShape_t shape, float t)
{
    /* Clamp input */
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;

    /* Map t to a float index */
    float fidx = t * (float)(WAVETABLE_SIZE - 1);
    int   i    = (int)fidx;
    float frac = fidx - (float)i;

    /* Guard upper bound (shouldn't be needed after clamp, but be safe) */
    if (i >= WAVETABLE_SIZE - 1)
        return lut[shape][WAVETABLE_SIZE - 1];

    /* Linear interpolation */
    return lut[shape][i] + frac * (lut[shape][i + 1] - lut[shape][i]);
}

/* --------------------------------------------------------------------------
 * Wavetable_ShapeEnvelope
 *
 * Remaps a linear ADSR amplitude (0.0–1.0) through the LUT.
 *
 * The ADSR in envelope.c increments/decrements current_amplitude linearly
 * (e.g. += attack_rate each sample).  This makes attack and release sound
 * mechanical.  Passing the linear amplitude through an exponential LUT
 * converts it to a perceptually even curve with no extra state.
 *
 * Usage in Pipeline_Process():
 *
 *   float amp = Envelope_Update(&p->envelope);       // linear 0.0–1.0
 *   amp       = Wavetable_ShapeEnvelope(&p->wt, amp);// shaped 0.0–1.0
 *   signal   *= amp;
 * -------------------------------------------------------------------------- */
float Wavetable_ShapeEnvelope(Wavetable_t *wt, float linear_amplitude)
{
    float shaped = Wavetable_Lookup(wt->shape, linear_amplitude);

    /* Blend between dry (linear) and shaped output */
    return (1.0f - wt->mix) * linear_amplitude + wt->mix * shaped;
}

/* --------------------------------------------------------------------------
 * Wavetable_ProcessSample
 *
 * Maps an audio sample through the LUT for PWM smoothing.
 *
 * The pipeline operates in a signed float range (~-32768 to +32767).
 * This function:
 *   1. Normalises the sample to [-1.0, +1.0]
 *   2. Preserves the sign (the LUT only covers 0.0–1.0)
 *   3. Looks up the absolute value through the table
 *   4. Re-applies the sign and restores to pipeline range
 *
 * With WT_SHAPE_EXPONENTIAL this soft-maps the quantised PWM steps onto
 * a smooth curve, reducing the harmonic content that makes PWM output
 * sound stepped and robotic.
 *
 * The mix parameter blends dry and shaped signals so you can dial in
 * as much smoothing as needed without losing transient punch.
 * -------------------------------------------------------------------------- */
float Wavetable_ProcessSample(Wavetable_t *wt, float sample)
{
    /* Normalise to [-1.0, +1.0] */
    float norm = sample / 32768.0f;

    /* Clamp (signal can exceed range if echo feedback accumulates) */
    if (norm >  1.0f) norm =  1.0f;
    if (norm < -1.0f) norm = -1.0f;

    /* Preserve sign, look up magnitude */
    float sign   = (norm >= 0.0f) ? 1.0f : -1.0f;
    float mag    = norm * sign;                         /* abs(norm) */
    float shaped = Wavetable_Lookup(wt->shape, mag);

    /* Blend dry and shaped, restore sign and pipeline range */
    float out = (1.0f - wt->mix) * mag + wt->mix * shaped;
    return out * sign * 32768.0f;
}
