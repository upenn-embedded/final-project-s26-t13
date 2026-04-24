/* AUDIO_PIPELINE.H
 * Modular synthesizer pipeline with reorderable preset chains.
 * Presets define which modules run and in what order.
 */

#ifndef AUDIO_PIPELINE_H
#define AUDIO_PIPELINE_H

/* HAL types and stdbool (needed for bool in Pipeline_Process signature) */
#include <stdint.h>
#include <stdbool.h>
#include "envelope.h"
#include "echo.h"
#include "wavetable.h"

/* --------------------------------------------------------------------------
 * Input source selection
 * -------------------------------------------------------------------------- */
typedef enum {
    SOURCE_CV  = 0   // Hardware CV/gate input
} InputSource_t;

/* --------------------------------------------------------------------------
 * Module identifiers — one entry per DSP block in the system.
 * Add new modules here; no other enum or struct needs changing.
 * -------------------------------------------------------------------------- */
typedef enum {
    MOD_NONE       = 0,
    MOD_ECHO,
    MOD_ENVELOPE,
    MOD_WAVETABLE,      /* LUT-based curve shaper — PWM smoothing / env shaping */
    MOD_COUNT           /* keep last */
} ModuleID_t;

/* --------------------------------------------------------------------------
 * Preset: an ordered chain of up to PRESET_MAX_MODULES modules.
 * Chains are terminated by MOD_NONE.
 * -------------------------------------------------------------------------- */
#define PRESET_MAX_MODULES  4

typedef struct {
    ModuleID_t chain[PRESET_MAX_MODULES];
} Preset_t;

/* --------------------------------------------------------------------------
 * Pipeline state
 * -------------------------------------------------------------------------- */
typedef struct {
    /* DSP modules */
    ADSR_t      envelope;
    Echo_t      echo;
    Wavetable_t wavetable;

    /* One-pole IIR low-pass filter — applied after every preset chain.
     * lp_state: per-sample memory.
     * lp_coeff: cutoff coefficient set by the filter pot (0.02=dark, 0.99=open). */
    float    lp_state;
    float    lp_coeff;
    uint32_t dither_n;

    /* Routing */
    uint8_t  active_preset;
} AudioPipeline_t;

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */
void  Pipeline_Init(AudioPipeline_t *p);
float Pipeline_Process(AudioPipeline_t *p, float input_sample, bool hardware_gate);
void  Pipeline_SetPreset(AudioPipeline_t *p, uint8_t preset_id);

/* Apply the full SynthParams packet received over UART.
 * Knobs: attack (0), decay (1), echo_time (2), filter_cutoff (3).
 * Echo feedback is preset-specific — see Pipeline_SetPreset(). */
void  Pipeline_ApplyParams(AudioPipeline_t *p,
                           uint8_t preset_id,
                           uint8_t attack,
                           uint8_t decay,
                           uint8_t time_val,
                           uint8_t filter_cutoff);

#endif /* AUDIO_PIPELINE_H */
