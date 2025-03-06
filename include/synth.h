#ifndef SYNTH_H
#define SYNTH_H

#include <3ds.h>

#include "audio_utils.h"
#include "envelope.h"
#include "oscillators.h"

typedef struct {
    Envelope* env;
    PolyBLEPOscillator* osc;
} SubSynth;

extern void fillSubSynthAudiobuffer(ndspWaveBuf* waveBuf, size_t size, SubSynth* subsynth,
                                    float synthvol, int chan_id);

#endif  // SYNTH_H