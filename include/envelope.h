#ifndef ENVELOPE_H
#define ENVELOPE_H

#ifdef TESTING
#include "../tests/mock_3ds.h"
#else
#include <3ds/types.h>
#endif

#define MAX_ENVELOPE_DURATION_MS 5000

typedef enum { ENV_ON = 1, ENV_OFF = 0 } EnvGate;

typedef struct {
    EnvGate gate;
    size_t  env_pos; // Changed from int to size_t
    float  *env_buffer;
    float   sr;
    size_t  buffer_size; // Added to track allocated buffer size

    // Env params
    int   atk;
    int   dec;
    int   sus_time;
    float sus_level;
    int   rel;
    int   dur;
} Envelope;

extern Envelope defaultEnvelopeStruct(float sample_rate);

extern void triggerEnvelope(Envelope *env);

extern void renderEnvBuffer(Envelope *env);

extern bool updateAttack(Envelope *env, int attack);

extern bool updateDecay(Envelope *env, int decay);

extern bool updateSustain(Envelope *env, float sustain);

extern bool updateDuration(Envelope *env, int dur_ms);

extern bool updateRelease(Envelope *env, int release);

extern void updateEnvelope(Envelope *env, int attack, int decay, float sustain, int release,
                           int dur_ms);

extern float nextEnvelopeSample(Envelope *env);

#endif // ENVELOPE_H