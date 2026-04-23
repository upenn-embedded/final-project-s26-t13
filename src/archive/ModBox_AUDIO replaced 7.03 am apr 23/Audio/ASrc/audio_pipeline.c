/* AUDIO_PIPELINE.H
 * takes in input, allows source selection
 */

#include "audio_pipeline.h"
#include "envelope.h"

void Pipeline_Init(AudioPipeline_t *p) {
    Envelope_Init(&p->envelope);
    Echo_Init(&p->echo, 500);
    Discretizer_Init(&p->discretizer, 8, 2);
    p->source = SOURCE_CV;
    p->active_preset = 0;
}

float Pipeline_Process(AudioPipeline_t *p, float input_sample, bool hardware_gate) {
    float signal = input_sample;
    bool gate = false;

    // 1. INPUT ROUTING & PRE-PROCESSING
    if (p->source == SOURCE_CV) {
        // VCO Mode
        gate = hardware_gate;
    } else {
        // MIC Mode
        // MANDATORY: Route through Discretizer first as requested
        signal = Discretizer_Process(&p->discretizer, signal);

        // Keep gate open for mic so it doesn't cut out
        gate = true;
    }

    // 2. ENVELOPE LOGIC
    if (gate && p->envelope.state == IDLE) {
        Envelope_Trigger(&p->envelope);
    } else if (!gate && p->envelope.state != IDLE && p->envelope.state != RELEASE) {
        Envelope_Release(&p->envelope);
    }
    float current_env_val = Envelope_Update(&p->envelope);

    // 3. PRESET EFFECTS (Echo, etc.)
    switch (p->active_preset) {
        case 1: // Echo Path
            signal = Echo_Process(&p->echo, signal);
            break;
        case 2: // Lo-Fi Path
            // Maybe add extra filter here
            break;
    }

    // 4. FINAL OUTPUT
    return signal * current_env_val;
}

void Pipeline_SetSource(AudioPipeline_t *p, InputSource_t source) {
    p->source = source;
}

void Pipeline_SetPreset(AudioPipeline_t *p, uint8_t preset_id) {
    p->active_preset = preset_id;
}
