#ifndef NOISE_SYNTH_H
#define NOISE_SYNTH_H

#include "envelope.h"
#include <3ds/types.h>
#include <3ds/ndsp/ndsp.h>

typedef struct {
    Envelope *env;
    u16       lfsr_register;
} NoiseSynth;

void fillNoiseSynthAudiobuffer(ndspWaveBuf *waveBuf, size_t size, NoiseSynth *noiseSynth);

#endif // NOISE_SYNTH_H
