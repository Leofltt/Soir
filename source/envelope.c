#include "envelope.h"

#ifdef TESTING
#include "../tests/mock_3ds.h"
#else
#include <3ds/allocator/linear.h> // Updated path
#include <3ds/types.h>
#endif
#include <string.h>

#include <string.h> // For memset

Envelope defaultEnvelopeStruct(float sample_rate) {
    Envelope env = { .atk            = 50,
                     .dec            = 300,
                     .rel            = 100,
                     .dur            = 1000,
                     .sus_level      = 0.8,
                     .sus_time       = 550,
                     .gate           = ENV_OFF,
                     .env_pos        = 0,
                     .sr             = sample_rate,
                     .env_buffer     = NULL,
                     .pending_buffer = NULL,
                     .buffer_size    = 0 };

    // Pre-allocate buffers to the maximum possible size
    size_t max_size    = (size_t) (MAX_ENVELOPE_DURATION_MS * env.sr * 0.001) + 1; // +1 for safety
    env.env_buffer     = (float *) linearAlloc(max_size * sizeof(float));
    env.pending_buffer = (float *) linearAlloc(max_size * sizeof(float));

    if (env.env_buffer && env.pending_buffer) {
        env.buffer_size = max_size;
        memset(env.env_buffer, 0, max_size * sizeof(float));
        memset(env.pending_buffer, 0, max_size * sizeof(float));
    } else {
        // Allocation failed, ensure both are NULL
        if (env.env_buffer)
            linearFree(env.env_buffer);
        if (env.pending_buffer)
            linearFree(env.pending_buffer);
        env.env_buffer     = NULL;
        env.pending_buffer = NULL;
        env.buffer_size    = 0;
    }

    LightLock_Init(&env.lock);
    atomic_init(&env.buffer_is_pending, false);

    return env;
};

void triggerEnvelope(Envelope *env) {
    env->env_pos = 0;
    env->gate    = ENV_ON;
};

// Renders the envelope shape into the provided buffer
static void renderEnvToBuffer(Envelope *env, float *buffer) {
    if (!env || !buffer) {
        return;
    }

    int   x   = 0;
    float y   = 0.0f;
    float inc = 0.0f;

    // --- Attack Phase ---
    if (env->atk > 0) {
        inc = 1.0f / (float) env->atk;
    } else {
        inc = 0.0f;
    }
    for (int i = 0; i < env->atk; i++) {
        if (x < env->buffer_size)
            buffer[x] = y;
        y += inc;
        x++;
    }
    y = 1.0f;

    // --- Decay Phase ---
    if (env->dec > 0) {
        inc = (env->sus_level - 1.0f) / (float) env->dec;
    } else {
        inc = 0.0f;
    }
    for (int i = 0; i < env->dec; i++) {
        if (x < env->buffer_size)
            buffer[x] = y;
        y += inc;
        x++;
    }
    y = env->sus_level;

    // --- Sustain Phase ---
    int sus_time_safe = (env->sus_time < 0) ? 0 : env->sus_time;
    for (int i = 0; i < sus_time_safe; i++) {
        if (x < env->buffer_size)
            buffer[x] = y;
        x++;
    }

    // --- Release Phase ---
    if (env->rel > 0) {
        inc = (0.0f - env->sus_level) / (float) env->rel;
    } else {
        inc = 0.0f;
    }
    for (int i = 0; i < env->rel; i++) {
        if (x < env->buffer_size)
            buffer[x] = y;
        y += inc;
        x++;
    }
    y = 0.0f;

    // Fill any remaining buffer with 0s
    while (x < env->buffer_size) {
        buffer[x] = 0.0f;
        x++;
    }
}

void renderEnvBuffer(Envelope *env) {
    if (!env || !env->pending_buffer) {
        return;
    }
    renderEnvToBuffer(env, env->pending_buffer);
    atomic_store(&env->buffer_is_pending, true);
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
    bool updated = false;

    // Clamp the duration to our pre-allocated buffer size
    if (dur_ms > MAX_ENVELOPE_DURATION_MS) {
        dur_ms = MAX_ENVELOPE_DURATION_MS;
    }

    if (env->dur != dur_ms) {
        env->dur = dur_ms;
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
    if (!env) {
        return 0.0f;
    }

    // Check if a new buffer is ready to be swapped
    if (atomic_load(&env->buffer_is_pending)) {
        LightLock_Lock(&env->lock);
        // Double check after acquiring the lock
        if (env->buffer_is_pending) {
            float *temp         = env->env_buffer;
            env->env_buffer     = env->pending_buffer;
            env->pending_buffer = temp;
            atomic_store(&env->buffer_is_pending, false);
        }
        LightLock_Unlock(&env->lock);
    }

    if (!env->env_buffer || env->env_pos >= env->buffer_size) {
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

void Envelope_deinit(Envelope *env) {
    if (env) {
        if (env->env_buffer) {
            linearFree(env->env_buffer);
            env->env_buffer = NULL;
        }
        if (env->pending_buffer) {
            linearFree(env->pending_buffer);
            env->pending_buffer = NULL;
        }
    }
}
