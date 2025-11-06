#include "envelope.h"

#ifdef TESTING
#include "../tests/mock_3ds.h"
#else
#include <3ds/allocator/linear.h> // Updated path
#include <3ds/types.h>
#endif

Envelope defaultEnvelopeStruct(float sample_rate) {
    Envelope env = { .atk         = 50,
                     .dec         = 300,
                     .rel         = 100,
                     .dur         = 1000,
                     .sus_level   = 0.8,
                     .sus_time    = 550,
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

    int   x   = 0;
    float y   = 0.0f;
    float inc = 0.0f;

    // --- Attack Phase ---
    // Handle 0 attack to prevent divide-by-zero
    if (env->atk > 0) {
        inc = 1.0f / (float) env->atk;
    } else {
        inc = 0.0f; // No attack
    }
    // Loop *atk* times
    for (int i = 0; i < env->atk; i++) {
        if (x < env->buffer_size)
            env->env_buffer[x] = y;
        y += inc;
        x++;
    }
    y = 1.0f; // Ensure we are at peak

    // --- Decay Phase ---
    // Handle 0 decay
    if (env->dec > 0) {
        inc = (env->sus_level - 1.0f) / (float) env->dec;
    } else {
        inc = 0.0f; // No decay
    }
    // Loop *dec* times
    for (int i = 0; i < env->dec; i++) {
        if (x < env->buffer_size)
            env->env_buffer[x] = y;
        y += inc;
        x++;
    }
    y = env->sus_level; // Ensure we are at sustain level

    // --- Sustain Phase ---
    // Loop *sus_time* times
    // (Ensure sus_time is not negative)
    int sus_time_safe = (env->sus_time < 0) ? 0 : env->sus_time;
    for (int i = 0; i < sus_time_safe; i++) {
        if (x < env->buffer_size)
            env->env_buffer[x] = y;
        x++;
    }
    // y is still env->sus_level

    // --- Release Phase ---
    // Handle 0 release
    if (env->rel > 0) {
        inc = (0.0f - env->sus_level) / (float) env->rel;
    } else {
        inc = 0.0f; // No release
    }
    // Loop *rel* times
    for (int i = 0; i < env->rel; i++) {
        if (x < env->buffer_size)
            env->env_buffer[x] = y;
        y += inc;
        x++;
    }
    y = 0.0f; // Ensure we end at 0

    // Fill any remaining buffer with 0s (if sustain_time was negative)
    while (x < env->buffer_size) {
        env->env_buffer[x] = 0.0f;
        x++;
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
    int  dur_samples  = (int) (env->dur * env->sr * 0.001);
    int  sustain_time = dur_samples - (env->atk + env->dec + env->rel);
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
        env->dur = new_size;
        updated  = true;
    }
    return updated;
}

void updateEnvelope(Envelope *env, int attack, int decay, float sustain, int release, int dur_ms) {
    updateDuration(env, dur_ms);
    updateAttack(env, attack);
    updateDecay(env, decay);
    updateRelease(env, release);
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
        if (env->env_pos < env->buffer_size) {
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
