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
    int16_t signal = 0;
    bool gate = false;

    // 1. Source Selection
    if (p->source == SOURCE_CV) {
        signal = input_sample;
        gate = hardware_gate;
    } else {
        signal = Discretizer_Process(&p->discretizer, input_sample);
        gate = 1; // Temporarily force the gate open so the envelope always plays
    }

    // 2. Process Envelope (Always runs, output sent to PA5)
    //Envelope_Process(&p->envelope, gate);
    // 1. Check the gate to trigger or release the ADSR
    if (gate == 1 && p->envelope.state == IDLE) {
    	Envelope_Trigger(&p->envelope);
    } else if (gate == 0 && p->envelope.state != IDLE && p->envelope.state != RELEASE) {
    	Envelope_Release(&p->envelope);
    }
    // 2. Calculate the current amplitude for this audio sample
    float current_env_val = Envelope_Update(&p->envelope);
    (void)current_env_val; // <--- Add this line to silence the warning!
    // (You can then multiply your audio signal by current_env_val to apply the volume envelope!)
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
        	p->envelope.release_rate = 0.001f;
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
