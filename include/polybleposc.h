#ifndef POLYBLEPOSC_H
#define POLYBLEPOSC_H

#include <math.h>

#include "audio_utils.h"

typedef enum { SINE = 0, SQUARE = 1, SAW = 2 } Waveform;

extern const char* waveform_names[];

typedef struct {
    float frequency;
    float samplerate;
    Waveform waveform;
    float phase_inc;
    float phase;
} PolyBLEPOscillator;

extern void setWaveform(PolyBLEPOscillator* osc, int wf_idx);
;

extern void setOscFrequency(PolyBLEPOscillator* osc, float frequency);

extern float oscillate(PolyBLEPOscillator* osc);

extern float polyBLEP(PolyBLEPOscillator* osc, float t);

extern float nextOscillatorSample(PolyBLEPOscillator* osc);

#endif  // POLYBLEPOSC_H