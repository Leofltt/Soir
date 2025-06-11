#include "envelope.h"

#include <3ds.h>

Envelope defaultEnvelopeStruct(float sample_rate) {
    Envelope env = { .atk         = 10,
                     .dec         = 10,
                     .rel         = 300,
                     .dur         = 400,
                     .sus_level   = 0.8,
                     .sus_time    = 80,
                     .gate        = ENV_OFF,
                     .env_pos     = 0,
                     .sr          = sample_rate,
                     .env_buffer  = NULL,
                     .buffer_size = 0 };

    size_t initial_size = (size_t) (env.dur * env.sr * 0.001);
    env.env_buffer      = (float *) linearAlloc(initial_size * sizeof(float));
    if (env.env_buffer) {
        env.buffer_size = initial_size;
    }
    return env;
};

void triggerEnvelope(Envelope *env) {
    env->env_pos = 0;
    env->gate    = ENV_ON;
};

void renderEnvBuffer(Envelope *env) {
    if (!env || !env->env_buffer) {
        return;
    }

    int   x = 0;
    float y = 0.;

    float inc = 1. / (float) env->atk;
    for (int i = 0; i <= env->atk; i++) {
        env->env_buffer[i] = y;
        y += inc;
    }

    inc = env->sus_level - 1. / (float) env->dec;
    x += env->atk;
    for (int i = 0; i <= env->dec; i++) {
        env->env_buffer[i + x] = y;
        y += inc;
    }

    x += env->dec;
    for (int i = 0; i <= env->sus_time; i++) {
        env->env_buffer[i + x] = y;
    }

    inc = 0. - env->sus_level / (float) env->rel;
    x += env->sus_time;
    for (int i = 0; i <= env->rel; i++) {
        env->env_buffer[i + x] = y;
        y += inc;
    }
};

bool updateAttack(Envelope *env, int attack) {
    int  new_atk = attack * env->sr * 0.001;
    bool updated = false;
    if (env->atk != new_atk) {
        env->atk = new_atk;
        updated  = true;
    }
    return updated;
};

bool updateDecay(Envelope *env, int decay) {
    int  new_decay = decay * env->sr * 0.001;
    bool updated   = false;
    if (env->dec != new_decay) {
        env->dec = new_decay;
        updated  = true;
    }
    return updated;
};

bool updateSustain(Envelope *env, float sustain) {
    int  sustain_time = env->dur - (env->atk + env->dec + env->rel);
    bool updated      = false;
    if (env->sus_level != sustain) {
        env->sus_level = sustain;
        updated        = true;
    }
    if (env->sus_time != sustain_time) {
        env->sus_time = sustain_time > 0 ? sustain_time : 0;
        updated       = true;
    }
    return updated;
};

bool updateRelease(Envelope *env, int release) {
    int  new_rel = release * env->sr * 0.001;
    bool updated = false;
    if (env->rel != new_rel) {
        env->rel = new_rel;
        updated  = true;
    }
    return updated;
};

bool updateDuration(Envelope *env, int dur_ms) {
    size_t new_size = (size_t) (dur_ms * env->sr * 0.001);
    bool   updated  = false;

    if (env->dur != dur_ms) {
        if (new_size > env->buffer_size) {
            float *new_buffer = (float *) linearAlloc(new_size * sizeof(float));
            if (new_buffer) {
                if (env->env_buffer) {
                    linearFree(env->env_buffer);
                }
                env->env_buffer  = new_buffer;
                env->buffer_size = new_size;
            }
        }
        env->dur = dur_ms;
        updated  = true;
    }
    return updated;
}

void updateEnvelope(Envelope *env, int attack, int decay, float sustain, int release, int dur_ms) {
    updateAttack(env, attack);
    updateDecay(env, decay);
    updateRelease(env, release);
    updateDuration(env, dur_ms);
    updateSustain(env, sustain);
    renderEnvBuffer(env);
};

float nextEnvelopeSample(Envelope *env) {
    if (!env || !env->env_buffer || env->env_pos >= env->buffer_size) {
        return 0.0f;
    }

    float env_value;
    switch (env->gate) {
    case ENV_OFF: {
        env_value = 0.;
        break;
    }
    case ENV_ON: {
        if (env->env_pos < env->dur) {
            env_value = env->env_buffer[env->env_pos];
            env->env_pos++;
        } else {
            env_value    = 0.;
            env->gate    = ENV_OFF;
            env->env_pos = 0;
        }
        break;
    }
    default: {
        env->gate    = ENV_OFF;
        env_value    = 0.;
        env->env_pos = 0;
        break;
    }
    }
    return env_value;
}
