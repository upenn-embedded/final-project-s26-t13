/* AUDIO_PIPELINE.H
 * defines presets and input types
 */

#ifndef AUDIO_PIPELINE_H
#define AUDIO_PIPELINE_H

#include <stdint.h>
#include <stdbool.h>
#include "main.h"
#include "audio.h"
#include "envelope.h"
#include "echo.h"
#include "discretizer.h"

typedef enum {
    SOURCE_CV  = 0,
    SOURCE_MIC = 1
} InputSource_t;

typedef struct {
    ADSR_t  		 envelope;
    Echo_t      	 echo;
    Discretizer_t    discretizer;
    InputSource_t    source;
    uint8_t          active_preset;
    uint8_t          knob_value;
} AudioPipeline_t;

void    Pipeline_Init(AudioPipeline_t *p);
int16_t Pipeline_Process(AudioPipeline_t *p, int16_t input_sample, bool hardware_gate);
void    Pipeline_SetSource(AudioPipeline_t *p, InputSource_t source);
void    Pipeline_SetPreset(AudioPipeline_t *p, uint8_t preset_id);

#endif
