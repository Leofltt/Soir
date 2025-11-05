#include "fm_osc.h"
#include "polybleposc.h"
#include <math.h>

void FMOperator_set_carrier_frequency(FMOperator *op, float freq) {
    if (!op || !op->carrier)
        return;
    setOscFrequency(op->carrier, freq);
}

void FMOperator_set_modulator_frequency_ratio(FMOperator *op, float ratio) {
    if (!op || !op->modulator || !op->carrier)
        return;
    setOscFrequency(op->modulator, op->carrier->frequency * ratio);
}

float nextFMOscillatorSample(FMOperator *op) {
    if (!op || !op->carrier || !op->modulator || !op->modEnvelope)
        return 0.0f;

    float mod_env_val = nextEnvelopeSample(op->modEnvelope);
    float mod_signal  = nextOscillatorSample(op->modulator) * op->modDepth * mod_env_val;

    float original_freq  = op->carrier->frequency;
    float modulated_freq = original_freq + mod_signal * op->modIndex * original_freq;

    setOscFrequency(op->carrier, modulated_freq);

    return nextOscillatorSample(op->carrier);
}

void FMOperator_set_mod_index(FMOperator *op, float index) {
    if (!op)
        return;
    op->modIndex = index;
}

void FMOperator_set_mod_depth(FMOperator *op, float depth) {
    if (!op)
        return;
    op->modDepth = depth;
}
