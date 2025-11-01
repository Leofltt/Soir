#ifndef SYNTH_H
#define SYNTH_H

#include "audio_utils.h"
#include "envelope.h"
#include "oscillators.h"

#ifndef TESTING
#include <3ds.h>
#endif

typedef struct {
    Envelope           *env;
    PolyBLEPOscillator *osc;
} SubSynth;

extern void fillSubSynthAudiobuffer(ndspWaveBuf *waveBuf, size_t size, SubSynth *subsynth);

#endif // SYNTH_H