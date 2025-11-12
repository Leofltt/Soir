#ifndef ENVELOPE_H
#define ENVELOPE_H

#ifdef TESTING
#include "../tests/mock_3ds.h"
#else
#include <3ds/types.h>
#endif

typedef enum {
    ENVELOPE_STATE_IDLE,
    ENVELOPE_STATE_ATTACK,
    ENVELOPE_STATE_DECAY,
    ENVELOPE_STATE_SUSTAIN,
    ENVELOPE_STATE_RELEASE,
} EnvelopeState;

typedef struct {
    EnvelopeState state;
    float         output;
    float         attack_rate;
    float         decay_rate;
    float         release_rate;
    float         sustain_level;
    float         sr;
    u32           env_pos;
    u32           dur_samples;
} Envelope;

extern Envelope defaultEnvelopeStruct(float sample_rate);

extern void triggerEnvelope(Envelope *env);

extern void updateEnvelope(Envelope *env, int attack_ms, int decay_ms, float sustain_level,
                           int release_ms, int dur_ms);

extern float nextEnvelopeSample(Envelope *env);

extern void releaseEnvelope(Envelope *env);

#endif // ENVELOPE_H