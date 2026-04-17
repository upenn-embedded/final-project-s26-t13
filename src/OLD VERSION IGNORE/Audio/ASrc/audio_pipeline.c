/* AUDIO_PIPELINE.H
 * takes in input, allows source selection
 */

#include "audio_pipeline.h"

void Pipeline_Init(AudioPipeline_t *p) {
    Envelope_Init(&p->envelope);
    Echo_Init(&p->echo);
    Discretizer_Init(&p->discretizer);
    p->source = SOURCE_CV;
    p->active_preset = 0;
}

int16_t Pipeline_Process(AudioPipeline_t *p, int16_t input_sample, bool hardware_gate) {
    int16_t signal = 0;
    bool gate = false;

    // 1. Source Selection
    if (p->source == SOURCE_CV) {
        signal = input_sample;
        gate = hardware_gate;
    } else {
        signal = Discretizer_Process(&p->discretizer, input_sample);
        gate = (p->discretizer.state == DISC_PLAYING);
    }

    // 2. Process Envelope (Always runs, output sent to PA5)
    Envelope_Process(&p->envelope, gate);

    // 3. Preset Effects Routing (Output sent to PA4)
    switch (p->active_preset) {
        case 0: // Preset 1: Clean Path (Input -> Filter -> VCA)
            break;
        case 1: // Preset 2: Echo Path (Input -> Echo -> Filter -> VCA)
            signal = Echo_Process(&p->echo, signal);
            break;
        case 2: // Preset 3: Lo-Fi (Short Echo)
            p->echo.delay_samples = 500;
            signal = Echo_Process(&p->echo, signal);
            break;
        case 3: // Preset 4: Long Tail (Echo + Slow Release)
            p->envelope.release_coeff = 0.001f;
            signal = Echo_Process(&p->echo, signal);
            break;
    }

    return signal;
}

void Pipeline_SetSource(AudioPipeline_t *p, InputSource_t source) {
    p->source = source;
}

void Pipeline_SetPreset(AudioPipeline_t *p, uint8_t preset_id) {
    p->active_preset = preset_id;
}
