#ifndef SYNTH_H
#define SYNTH_H

#include "audio_utils.h"
#include "envelope.h"
#include "oscillators.h"
#include "fm_osc.h"

#ifndef TESTING
#include <3ds.h>
#endif

typedef struct {
    Envelope           *env;
    PolyBLEPOscillator *osc;
} SubSynth;

typedef struct {
    Envelope   *carrierEnv;
    FMOperator *fm_op;
} FMSynth;

extern void fillSubSynthAudiobuffer(ndspWaveBuf *waveBuf, size_t size, SubSynth *subsynth);
extern void fillFMSynthAudiobuffer(ndspWaveBuf *waveBuf, size_t size, FMSynth *fm_synth);

#endif // SYNTH_H