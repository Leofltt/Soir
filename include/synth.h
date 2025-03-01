#ifndef SYNTH_H
#define SYNTH_H

#include "oscillators.h"
#include "envelope.h"
#include "audio_utils.h"

typedef struct {
    Envelope* env;
    PolyBLEPOscillator* osc;
} SubSynth;

extern void fillSubSynthAudiobuffer(void* audioBuffer, size_t size, SubSynth* subsynth, float synthvol);

#endif // SYNTH_H