#ifndef FM_OSC_H
#define FM_OSC_H

#include "envelope.h"
#include "polybleposc.h"

typedef struct {
    PolyBLEPOscillator *carrier;
    PolyBLEPOscillator *modulator;
    Envelope           *modEnvelope;
    float               modIndex;
    float               modDepth;
} FMOperator;

extern void setFMOscFrequency(FMOperator *osc, float frequency);

extern float nextFMOscillatorSample(FMOperator *osc);

extern void setModIndex(FMOperator *osc, float index);

extern void setModDepth(FMOperator *osc, float depth);

#endif // FM_OSC_H
