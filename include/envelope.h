#ifndef ENVELOPE_H
#define ENVELOPE_H

#include <stdbool.h>

typedef enum {
    ENV_ON = 1,
    ENV_OFF = 0
} EnvGate;

typedef struct {
    EnvGate gate;
    int env_pos;
    float* env_buffer;
    float sr;
    
    // Env params 
    int atk;
    int dec;
    int sus_time;
    float sus_level; 
    int rel;
    int dur; 
} Envelope;

extern void triggerEnvelope(Envelope* env);

extern void renderEnvBuffer(Envelope* env);

extern bool updateAttack(Envelope* env, int attack);

extern bool updateDecay(Envelope* env, int decay);

extern bool updateSustain(Envelope* env, float sustain);

extern bool updateDuration(Envelope* env, int dur_ms);

extern bool updateRelease(Envelope* env, int release);

extern void updateEnvelope(Envelope* env, int attack, int decay, float sustain, int release, int dur_ms);

extern float nextEnvelopeSample(Envelope* env);

#endif // ENVELOPE_H