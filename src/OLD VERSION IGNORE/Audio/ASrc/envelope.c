#include "envelope.h"

void Envelope_Init(ADSR_t* adsr) {
    adsr->state = IDLE;
    adsr->current_amplitude = 0.0f;
    // Default "Snappy" Settings
    adsr->attack_rate = 0.001f;
    adsr->decay_rate = 0.0005f;
    adsr->sustain_level = 0.7f;
    adsr->release_rate = 0.0002f;
}

void Envelope_Trigger(ADSR_t* adsr) {
    adsr->state = ATTACK;
}

void Envelope_Release(ADSR_t* adsr) {
    adsr->state = RELEASE;
}

float Envelope_Update(ADSR_t* adsr) {
    switch (adsr->state) {
        case ATTACK:
            adsr->current_amplitude += adsr->attack_rate;
            if (adsr->current_amplitude >= 1.0f) {
                adsr->current_amplitude = 1.0f;
                adsr->state = DECAY;
            }
            break;

        case DECAY:
            adsr->current_amplitude -= adsr->decay_rate;
            if (adsr->current_amplitude <= adsr->sustain_level) {
                adsr->current_amplitude = adsr->sustain_level;
                adsr->state = SUSTAIN;
            }
            break;

        case SUSTAIN:
            // Volume stays steady until Envelope_Release is called
            break;

        case RELEASE:
            adsr->current_amplitude -= adsr->release_rate;
            if (adsr->current_amplitude <= 0.0f) {
                adsr->current_amplitude = 0.0f;
                adsr->state = IDLE;
            }
            break;

        case IDLE:
        default:
            adsr->current_amplitude = 0.0f;
            break;
    }
    return adsr->current_amplitude;
}
