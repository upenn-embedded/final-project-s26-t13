/* AUDIO_PIPELINE.C
 * Modular synthesizer pipeline with reorderable preset chains.
 *
 * HOW PRESETS WORK
 * ----------------
 * Each preset is a small array (chain[]) of ModuleID_t values that lists
 * which DSP blocks to run and in what order.  Pipeline_Process() iterates
 * the chain and dispatches to each module in sequence.  To reorder modules,
 * change the chain — no switch-case surgery required.
 *
 * PRESET TABLE (edit here to add / reorder / remove modules per preset)
 * -----------------------------------------------------------------------
 *  Preset 0 – Clean          : Envelope only
 *  Preset 1 – Warm Echo      : Echo → Envelope → softclip/LP/dither
 *  Preset 2 – Lo-Fi Echo     : Echo (short) → Envelope
 *  Preset 3 – Long Tail      : Echo → Envelope (slow release)
 * -----------------------------------------------------------------------
 */

#include "audio_pipeline.h"
#include <stdint.h>

/* --------------------------------------------------------------------------
 * Preset table — data-driven module ordering.
 * Chains are processed left-to-right; MOD_NONE terminates early.
 * -------------------------------------------------------------------------- */
static const Preset_t preset_table[] = {
    /* 0 – Clean: straight to envelope */
    { .chain = { MOD_ENVELOPE,    MOD_NONE, MOD_NONE, MOD_NONE } },

    /* 1 – Echo: echo colours the signal before the envelope shapes it */
    { .chain = { MOD_ECHO,        MOD_ENVELOPE, MOD_NONE, MOD_NONE } },

    /* 2 – Lo-Fi Echo: short echo delay into envelope */
    { .chain = { MOD_ECHO, MOD_ENVELOPE, MOD_NONE, MOD_NONE } },

    /* 3 – Long Tail: echo feeds into a slow-release envelope */
    { .chain = { MOD_ECHO,        MOD_ENVELOPE, MOD_NONE, MOD_NONE } },
};

#define NUM_PRESETS  ((uint8_t)(sizeof(preset_table) / sizeof(preset_table[0])))

/* --------------------------------------------------------------------------
 * Helpers: scale 0-255 UART bytes to float ranges
 * -------------------------------------------------------------------------- */
static inline float byte_to_rate(uint8_t b, float min, float max)
{
    return min + ((float)b / 255.0f) * (max - min);
}

static inline uint32_t byte_to_samples(uint8_t b, uint32_t min, uint32_t max)
{
    return min + (uint32_t)(((float)b / 255.0f) * (float)(max - min));
}

/* --------------------------------------------------------------------------
 * Warmth helpers (used by preset 1)
 * -------------------------------------------------------------------------- */

/* Soft clipper — cubic waveshaper, models tube saturation.
 * Input and output are in the normalised [-1.0, +1.0] range.
 * Rounds off peaks instead of hard-clipping, removing harshness. */
static inline float softclip(float x)
{
    if (x >  1.0f) return  1.0f;
    if (x < -1.0f) return -1.0f;
    return x - (x * x * x) * 0.3333f;   /* x - x³/3  (tanh approximation) */
}

/* One-pole IIR low-pass — single multiply-add per sample.
 * coeff: 0.0 = no filtering (dry), higher = darker/warmer.
 * 0.15 rolls off gently above ~1kHz at 8kHz sample rate.          */
#define LP_COEFF  0.15f

static inline float lp_tick(float *state, float x)
{
    *state += LP_COEFF * (x - *state);
    return *state;
}

/* 1-bit rectangular dither — breaks up quantisation patterns.
 * Alternates +1/-1 each sample; crude but effective and free.      */
static inline float dither(uint32_t *n)
{
    *n += 1;
    return (*n & 1) ? 1.0f : -1.0f;
}

/* --------------------------------------------------------------------------
 * Pipeline_Init
 * -------------------------------------------------------------------------- */
void Pipeline_Init(AudioPipeline_t *p)
{
    Envelope_Init(&p->envelope);
    Echo_Init(&p->echo, 500);
    p->source        = SOURCE_CV;
    p->active_preset = 0;
    p->lp_state      = 0.0f;
    p->dither_n      = 0;
}

/* --------------------------------------------------------------------------
 * Pipeline_Process
 *
 * Walks the active preset's module chain in order.  Each module receives
 * the signal produced by the previous one, so reordering the chain[]
 * entries directly changes the signal path.
 * -------------------------------------------------------------------------- */
float Pipeline_Process(AudioPipeline_t *p, float input_sample, bool hardware_gate)
{
    float signal = input_sample;
    bool  gate   = hardware_gate;

    /* Guard against an out-of-range preset (e.g. after a bad UART packet) */
    uint8_t preset_idx = (p->active_preset < NUM_PRESETS)
                         ? p->active_preset : 0;

    const Preset_t *preset = &preset_table[preset_idx];

    /* -----------------------------------------------------------------------
     * Apply preset-specific parameter overrides BEFORE processing.
     * Keeps the dispatch loop below clean and module-agnostic.
     * --------------------------------------------------------------------- */
    if (preset_idx == 2) {
        /* Lo-Fi: use a short echo delay */
        p->echo.delay_samples = 100;
    }
    if (preset_idx == 3) {
        /* Long Tail: slow envelope release */
        p->envelope.release_rate = 0.001f;
    }

    /* -----------------------------------------------------------------------
     * Module dispatch loop — processes each slot in the preset chain.
     * --------------------------------------------------------------------- */
    for (int i = 0; i < PRESET_MAX_MODULES; i++)
    {
        switch (preset->chain[i])
        {
            /* --- End of chain --- */
            case MOD_NONE:
                goto pipeline_done;

            /* --- Echo / delay --- */
            case MOD_ECHO:
                signal = (float)Echo_Process(&p->echo, (int16_t)signal);
                break;

            /* --- Envelope (ADSR amplitude shaper) --- */
            case MOD_ENVELOPE:
                /* Trigger on gate rising edge */
                if (gate && p->envelope.state == IDLE)
                    Envelope_Trigger(&p->envelope);

                /* Release on gate falling edge */
                else if (!gate
                         && p->envelope.state != IDLE
                         && p->envelope.state != RELEASE)
                    Envelope_Release(&p->envelope);

                /* Multiply signal by current envelope amplitude */
                signal *= Envelope_Update(&p->envelope);
                break;

            default:
                break;
        }
    }

pipeline_done:

    /* -----------------------------------------------------------------------
     * Preset 1 — Warm Echo post-processing.
     * Runs AFTER the dispatch loop so echo repeats are warmed up too,
     * not just the dry signal.
     *
     * Chain: softclip → IIR low-pass → dither
     *   softclip  : cubic waveshaper, rounds off harsh peaks (tube-like)
     *   lp_tick   : one-pole IIR, darkens the top end sample-by-sample
     *   dither    : 1-bit rectangular, breaks up PWM quantisation steps
     * --------------------------------------------------------------------- */
    if (preset_idx == 1) {
        float norm = signal / 32768.0f;
        norm   = softclip(norm);
        norm   = lp_tick(&p->lp_state, norm);
        norm  += dither(&p->dither_n) / 32768.0f;
        signal = norm * 32768.0f;
    }

    return signal;
}

/* --------------------------------------------------------------------------
 * Pipeline_SetSource
 * -------------------------------------------------------------------------- */
void Pipeline_SetSource(AudioPipeline_t *p, InputSource_t source)
{
    p->source = source;
}

/* --------------------------------------------------------------------------
 * Pipeline_SetPreset
 * -------------------------------------------------------------------------- */
void Pipeline_SetPreset(AudioPipeline_t *p, uint8_t preset_id)
{
    if (preset_id < NUM_PRESETS)
        p->active_preset = preset_id;
}

/* --------------------------------------------------------------------------
 * Pipeline_ApplyParams
 *
 * Called from main.c whenever a new 7-byte UART packet arrives.
 * Maps the raw 0-255 parameter bytes to the correct internal ranges and
 * pushes them into every module so the pipeline is always in sync.
 * -------------------------------------------------------------------------- */
void Pipeline_ApplyParams(AudioPipeline_t *p,
                          uint8_t input_mode,
                          uint8_t preset_id,
                          uint8_t attack,
                          uint8_t release_val,
                          uint8_t time_val,
                          uint8_t feedback)
{
    /* Source selection: byte 0  (0 = CV, anything else = internal) */
    Pipeline_SetSource(p, (input_mode == 0) ? SOURCE_CV : SOURCE_CV);

    /* Preset / module order: byte 1 */
    Pipeline_SetPreset(p, preset_id);

    /* Envelope rates: scale 0-255 → slow (0.0001) to fast (0.05) */
    p->envelope.attack_rate  = byte_to_rate(attack,      0.0001f, 0.05f);
    p->envelope.release_rate = byte_to_rate(release_val, 0.0001f, 0.02f);

    /* Echo: delay time (byte 4) and feedback (byte 5)
     * delay_samples: 50 – 2000 samples
     * feedback:       0.0 – 0.9  */
    p->echo.delay_samples = byte_to_samples(time_val, 50, 2000);
    p->echo.feedback      = byte_to_rate(feedback, 0.0f, 0.9f);
}
