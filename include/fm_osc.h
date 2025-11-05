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

extern void  FMOperator_set_carrier_frequency(FMOperator *op, float freq);
extern void  FMOperator_set_modulator_frequency_ratio(FMOperator *op, float ratio);
extern float nextFMOscillatorSample(FMOperator *op);
extern void  FMOperator_set_mod_index(FMOperator *op, float index);
extern void  FMOperator_set_mod_depth(FMOperator *op, float depth);

#endif // FM_OSC_H
