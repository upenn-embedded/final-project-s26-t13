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
 * PRESET TABLE
 * -----------------------------------------------------------------------
 *  Preset 0 – Clean          : Envelope only (no effects, shaped amplitude)
 *  Preset 1 – Warm Echo      : Echo → Envelope
 *  Preset 2 – Lo-Fi Echo     : Echo (short) → Envelope
 *  Preset 3 – Long Tail      : Echo → Envelope
 *  Preset 4 – Envelope First : Envelope → Echo
 * -----------------------------------------------------------------------
 *
 * NOTE: All per-preset parameter overrides have been removed from
 * Pipeline_Process().  All rates and delays are now set exclusively by
 * Pipeline_ApplyParams() so the UART knobs always take effect.
 */

#include "audio_pipeline.h" /* already pulls in wavetable.h transitively */
#include <stdint.h>

/* --------------------------------------------------------------------------
 * Preset table — data-driven module ordering.
 * Chains are processed left-to-right; MOD_NONE terminates early.
 * -------------------------------------------------------------------------- */
static const Preset_t preset_table[] = {
    /* 0 – Clean: signal passes through unaffected, wavetable smooths PWM */
    { .chain = { MOD_WAVETABLE, MOD_NONE, MOD_NONE, MOD_NONE } },

    /* 1 – Warm Echo: echo colours the signal, envelope shapes, wavetable smooths output */
    { .chain = { MOD_ECHO, MOD_ENVELOPE, MOD_WAVETABLE, MOD_NONE } },

    /* 2 – Lo-Fi Echo: short echo into envelope — no wavetable smoothing for grittier tone.
     * Keep the time knob low (short delay) for lo-fi character. */
    { .chain = { MOD_ECHO, MOD_ENVELOPE, MOD_NONE, MOD_NONE } },

    /* 3 – Long Tail: echo into envelope with LUT shaping on the tail.
     * Turn release knob low for the long tail character. */
    { .chain = { MOD_ECHO, MOD_ENVELOPE, MOD_WAVETABLE, MOD_NONE } },

    /* 4 – Envelope First: gate+shape the input, then echo trails it with smooth curves */
    { .chain = { MOD_ENVELOPE, MOD_ECHO, MOD_WAVETABLE, MOD_NONE } },
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
 * Warmth helpers (available for post-processing)
 * -------------------------------------------------------------------------- */

/* Soft clipper — cubic waveshaper, models tube saturation.
 * Input and output are in the normalised [-1.0, +1.0] range.
 * Rounds off peaks instead of hard-clipping, removing harshness. */
static inline float softclip(float x)
{
    if (x >  1.0f) return  1.0f;
    if (x < -1.0f) return -1.0f;
    return x - (x * x * x) * 0.3333f;
}

/* One-pole IIR low-pass — single multiply-add per sample.
 * coeff: 0.02 = very dark, 0.99 = nearly transparent.
 * Applied to all presets; coeff is driven by the filter pot. */
static inline float lp_tick(float *state, float coeff, float x)
{
    *state += coeff * (x - *state);
    return *state;
}

/* 1-bit rectangular dither — breaks up quantisation patterns.
 * Alternates +1/-1 each sample; crude but effective and free. */
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
    Echo_Init(&p->echo, 200);
    Wavetable_Init(&p->wavetable, WT_SHAPE_EXPONENTIAL, 1.0f);  /* builds LUT */
    p->lp_state = 0.0f;
    p->lp_coeff = 0.7f;   /* moderately open on startup — pot sweeps from 0.02 to 0.99 */
    p->dither_n = 0;
    Pipeline_SetPreset(p, 0);          /* Clean default — signal passes through until user picks a preset */
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
     * Module dispatch loop — processes each slot in the preset chain.
     *
     * All module parameters (attack, release, delay, feedback) are set
     * exclusively by Pipeline_ApplyParams() so the UART knobs always work.
     * Do NOT add per-preset parameter overrides here.
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
                signal = Echo_Process(&p->echo, signal);
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

                /* Get linear amplitude, shape it through the LUT for a
                 * natural-sounding curve, then multiply into the signal.
                 * Wavetable_ShapeEnvelope() is a no-op if mix == 0.0. */
                {
                    float amp = Envelope_Update(&p->envelope);
                    amp = Wavetable_ShapeEnvelope(&p->wavetable, amp);
                    signal *= amp;
                }
                break;

            /* --- Wavetable LUT shaper (PWM smoothing) --- */
            case MOD_WAVETABLE:
                /* Maps the signal through a smooth curve to reduce the
                 * stepped/quantised character of the PWM output.
                 * Insert before or after other modules to taste:
                 *   before echo  → smooths the dry signal going into delay
                 *   after echo   → smooths the full wet+dry mix
                 *   after envelope → shapes the final amplitude-gated signal */
                signal = Wavetable_ProcessSample(&p->wavetable, signal);
                break;

            default:
                break;
        }
    }

pipeline_done:

    /* -----------------------------------------------------------------------
     * Post-processing — runs on every preset.
     *
     * 1. LP filter (all presets): coeff driven by filter pot (knob 3).
     *    Sweeping the pot from low to high opens the filter — the classic
     *    modular "filter sweep" sound.
     *
     * 2. Preset 1 extra warmth: softclip rounds off harsh peaks (tube-like);
     *    dither breaks up PWM quantisation patterns.
     * --------------------------------------------------------------------- */
    {
        float norm = signal / 32768.0f;
        if (norm >  1.0f) norm =  1.0f;
        if (norm < -1.0f) norm = -1.0f;
        norm   = lp_tick(&p->lp_state, p->lp_coeff, norm);
        signal = norm * 32768.0f;
    }

    if (preset_idx == 1) {
        float norm = signal / 32768.0f;
        norm  = softclip(norm);
        norm += dither(&p->dither_n) / 32768.0f;
        signal = norm * 32768.0f;
    }

    return signal;
}

/* --------------------------------------------------------------------------
 * Pipeline_SetPreset
 * -------------------------------------------------------------------------- */
void Pipeline_SetPreset(AudioPipeline_t *p, uint8_t preset_id)
{
    if (preset_id >= NUM_PRESETS) return;

    p->active_preset = preset_id;

    /* Wavetable curve per preset.
     * Logarithmic = fast pop (percussive). Exponential = gradual. Sine = S-curve. */
    static const WavetableShape_t preset_shapes[5] = {
        WT_SHAPE_EXPONENTIAL,   /* 0: Clean */
        WT_SHAPE_EXPONENTIAL,   /* 1: Warm Echo */
        WT_SHAPE_LOGARITHMIC,   /* 2: Lo-Fi Echo — percussive pop */
        WT_SHAPE_SINE,          /* 3: Long Tail */
        WT_SHAPE_LOGARITHMIC,   /* 4: Envelope First — punchy beat */
    };
    p->wavetable.shape = preset_shapes[preset_id];

    /* Echo feedback baked in per preset — frees the 4th pot for filter sweep.
     * 0.0=no repeats, 0.9=max (nearly self-oscillating). */
    static const float preset_feedback[5] = {
        0.0f,   /* 0: Clean — echo not in chain, irrelevant */
        0.45f,  /* 1: Warm Echo — warm trailing repeats */
        0.30f,  /* 2: Lo-Fi Echo — sparse gritty repeats */
        0.65f,  /* 3: Long Tail — heavy feedback for ambient build */
        0.40f,  /* 4: Envelope First — rhythmic beat echo */
    };
    p->echo.feedback = preset_feedback[preset_id];
}

/* --------------------------------------------------------------------------
 * Pipeline_ApplyParams
 *
 * Called from main.c whenever a new 5-byte UART packet arrives.
 * Maps the raw 0-255 parameter bytes to the correct internal ranges and
 * pushes them into every module so the pipeline is always in sync.
 *
 * This is the ONLY place module parameters should be set.
 * -------------------------------------------------------------------------- */
void Pipeline_ApplyParams(AudioPipeline_t *p,
                          uint8_t preset_id,
                          uint8_t attack,
                          uint8_t decay,
                          uint8_t time_val,
                          uint8_t filter_cutoff)
{
    /* Preset — also sets wavetable shape and echo feedback */
    Pipeline_SetPreset(p, preset_id);

    /* Envelope attack shapes the transient punch.
     * Envelope decay shapes note body length — with sustain_level near 0,
     * decay IS the effective note duration (drum-machine behaviour). */
    p->envelope.attack_rate = byte_to_rate(attack, 0.0001f, 0.05f);
    p->envelope.decay_rate  = byte_to_rate(decay,  0.0005f, 0.03f);

    /* Echo delay time: 50–2000 samples (~6ms–250ms at 8kHz) */
    p->echo.delay_samples = byte_to_samples(time_val, 50, 2000);
    /* echo.feedback is set per-preset in Pipeline_SetPreset() */

    /* LP filter cutoff: 0.02 (very dark) → 0.99 (open/bright).
     * Sweeping this pot is the classic modular synthesizer filter sound. */
    p->lp_coeff = byte_to_rate(filter_cutoff, 0.02f, 0.99f);
}
