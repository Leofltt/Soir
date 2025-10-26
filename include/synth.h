#ifndef SYNTH_H
#define SYNTH_H

#include "envelope.h"
#include "oscillators.h"
#include "track_parameters.h"

#ifndef TESTING
#include <3ds.h>
#endif

typedef struct SubSynth {
    PolyBLEPOscillator *osc;
    Envelope           *env;
} SubSynth;

struct Track;

extern void fillSubSynthAudiobuffer(struct Track *track, ndspWaveBuf *waveBuf, size_t size, float synthvol);

#endif // SYNTH_H