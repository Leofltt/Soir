#include "envelope.h"
#include <stdio.h>

#ifdef TESTING
#include "../tests/mock_3ds.h"
#else
#include <3ds/types.h>
#endif

Envelope defaultEnvelopeStruct(float sample_rate) {
    Envelope env = { .state         = ENVELOPE_STATE_IDLE,
                     .output        = 0.0f,
                     .attack_rate   = 0.0f,
                     .decay_rate    = 0.0f,
                     .release_rate  = 0.0f,
                     .sustain_level = 1.0f,
                     .sr            = sample_rate,
                     .env_pos       = 0,
                     .dur_samples   = 0 };
    return env;
}

void triggerEnvelope(Envelope *env) {
    env->state   = ENVELOPE_STATE_ATTACK;
    env->env_pos = 0;
}

void releaseEnvelope(Envelope *env) {
    if (env->state != ENVELOPE_STATE_IDLE) {
        env->state = ENVELOPE_STATE_RELEASE;
    }
}

void updateEnvelope(Envelope *env, int attack_ms, int decay_ms, float sustain_level, int release_ms,
                    int dur_ms) {
    if (!env)
        return;

    float sr_ms = env->sr / 1000.0f;

    env->attack_rate   = (attack_ms > 0) ? 1.0f / (attack_ms * sr_ms) : 0.0f;
    env->decay_rate    = (decay_ms > 0) ? 1.0f / (decay_ms * sr_ms) : 0.0f;
    env->sustain_level = sustain_level;
    env->release_rate  = (release_ms > 0) ? 1.0f / (release_ms * sr_ms) : 0.0f;
    env->dur_samples   = dur_ms * sr_ms;
}

float nextEnvelopeSample(Envelope *env) {
    if (!env)
        return 0.0f;

    switch (env->state) {
    case ENVELOPE_STATE_IDLE:
        env->output = 0.0f;
        break;
    case ENVELOPE_STATE_ATTACK:
        env->output += env->attack_rate;
        if (env->output >= 1.0f) {
            env->output = 1.0f;
            env->state  = ENVELOPE_STATE_DECAY;
        }
        break;
    case ENVELOPE_STATE_DECAY:
        env->output -= env->decay_rate;
        if (env->output <= env->sustain_level) {
            env->output  = env->sustain_level;
            env->state   = ENVELOPE_STATE_SUSTAIN;
            env->env_pos = 0;
        }
        break;
    case ENVELOPE_STATE_SUSTAIN:
        env->env_pos++;
        if (env->env_pos >= env->dur_samples) {
            env->state = ENVELOPE_STATE_RELEASE;
        }
        break;
    case ENVELOPE_STATE_RELEASE:
        env->output -= env->release_rate;
        if (env->output <= 0.0f) {
            env->output = 0.0f;
            env->state  = ENVELOPE_STATE_IDLE;
        }
        break;
    }
    return env->output;
}
