#ifndef POLYBLEPOSC_H
#define POLYBLEPOSC_H

#include "audio_utils.h"

#include <math.h>

typedef enum { SINE = 0, SQUARE = 1, SAW = 2, TRIANGLE = 3 } Waveform;

#define WAVEFORM_COUNT 4

extern const char *waveform_names[];

typedef struct {
    float    frequency;
    float    samplerate;
    Waveform waveform;
    float    phase_inc;
    float    phase;
    float    pulse_width;
} PolyBLEPOscillator;

extern void setWaveform(PolyBLEPOscillator *osc, int wf_idx);
extern void setPulseWidth(PolyBLEPOscillator *osc, float pulse_width);

extern void setOscFrequency(PolyBLEPOscillator *osc, float frequency);

extern float nextOscillatorSample(PolyBLEPOscillator *osc);

extern float polyBLEP(PolyBLEPOscillator *osc, float t);

#endif // POLYBLEPOSC_H