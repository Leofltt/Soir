#ifndef NOISE_SYNTH_H
#define NOISE_SYNTH_H

#include "envelope.h"
#include <3ds/types.h>
#include <3ds/ndsp/ndsp.h>

typedef struct {
    int   env_atk;
    int   env_dec;
    float env_sus_level;
    int   env_rel;
    int   env_dur; // Duration for triggered envelopes
} NoiseSynthParameters;

typedef struct {
    Envelope *env;
    u16       lfsr_register;
} NoiseSynth;

NoiseSynthParameters defaultNoiseSynthParameters();

void fillNoiseSynthAudiobuffer(ndspWaveBuf *waveBuf, size_t size, NoiseSynth *noiseSynth);

#endif // NOISE_SYNTH_H
