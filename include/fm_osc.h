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

extern void  FMOpSetCarrierFrequency(FMOperator *op, float freq);
extern void  FMOpSetModRatio(FMOperator *op, float ratio);
extern float nextFMOscillatorSample(FMOperator *op);
extern void  FMOpSetModIndex(FMOperator *op, float index);
extern void  FMOperator_set_mod_depth(FMOperator *op, float depth);

#endif // FM_OSC_H
