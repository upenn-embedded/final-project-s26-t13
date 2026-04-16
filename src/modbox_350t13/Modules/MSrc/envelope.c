/* ENVELOPE GENERATOR
 *
 * When gate is high, envelope generator targets attack, when gate is low,
 * envelope generator targets 0, hence ADSR (Attack, Decay, Sustain, Release)
 *
 * Typically used on VCA to create notes that have distinct beginning, middle, end
 */

#include "envelope.h"

#define ENV_MAX_OUT 4095.0f

void Envelope_Init(EnvelopeState_t *env) {
    env->level = 0.0f;
    env->attack_coeff = 0.01f;
    env->release_coeff = 0.005f;
    env->output = 0;
    env->bypassed = false;
}

uint16_t Envelope_Process(EnvelopeState_t *env, bool gate) {
    if (env->bypassed) {
        env->output = 4095; // Full open if bypassed
        return env->output;
    }

    /* Generator Logic:
       If gate is true, target is 1.0 (Attack)
       If gate is false, target is 0.0 (Release) */
    float target = gate ? 1.0f : 0.0f;

    if (target > env->level) {
        env->level += env->attack_coeff * (target - env->level);
    } else {
        env->level += env->release_coeff * (target - env->level);
    }

    /* Clamp for safety */
    if (env->level > 1.0f) env->level = 1.0f;
    if (env->level < 0.0f) env->level = 0.0f;

    env->output = (uint16_t)(env->level * ENV_MAX_OUT);
    return env->output;
}

void Envelope_SetParam(EnvelopeState_t *env, uint8_t knob_value) {
    float t = (float)knob_value / 255.0f;
    /* Cubic scaling makes the knob feel more musical (more control over fast speeds) */
    env->attack_coeff  = 0.0001f + (t * t * t) * 0.5f;
    env->release_coeff = 0.0001f + (t * t * t) * 0.1f;
}

void Envelope_SetBypassed(EnvelopeState_t *env, bool bypassed) {
    env->bypassed = bypassed;
}
