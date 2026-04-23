/* AUDIO_PIPELINE.H
 * Modular synthesizer pipeline with reorderable preset chains.
 * Presets define which modules run and in what order.
 */

#ifndef AUDIO_PIPELINE_H
#define AUDIO_PIPELINE_H

#include <stdint.h>
#include <stdbool.h>
#include "envelope.h"
#include "echo.h"

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
    MOD_NONE     = 0,
    MOD_ECHO,
    MOD_ENVELOPE,
    MOD_COUNT        /* keep last */
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
    ADSR_t   envelope;
    Echo_t   echo;

    /* Preset 1 warmth processing state
     * lp_state: one-pole IIR low-pass memory (persists sample-to-sample)
     * dither_n: counter for 1-bit rectangular dither                    */
    float    lp_state;
    uint32_t dither_n;

    /* Routing */
    uint8_t       active_preset;
} AudioPipeline_t;

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */
void  Pipeline_Init(AudioPipeline_t *p);
float Pipeline_Process(AudioPipeline_t *p, float input_sample, bool hardware_gate);
void  Pipeline_SetPreset(AudioPipeline_t *p, uint8_t preset_id);

/* Apply the full SynthParams packet received over UART.
 * Call this whenever new_data_flag fires in main. */
void  Pipeline_ApplyParams(AudioPipeline_t *p,
                           uint8_t preset_id,
                           uint8_t attack,
                           uint8_t release_val,
                           uint8_t time_val,
                           uint8_t feedback);

#endif /* AUDIO_PIPELINE_H */
