#include "fm_osc.h"
#include "polybleposc.h"
#include <math.h>

void FMOpSetCarrierFrequency(FMOperator *op, float freq) {
    if (!op || !op->carrier)
        return;
    setOscFrequency(op->carrier, freq);
    op->base_frequency = freq;
}

void FMOpSetModRatio(FMOperator *op, float ratio) {
    if (!op || !op->modulator || !op->carrier)
        return;
    setOscFrequency(op->modulator, op->base_frequency * ratio);
}

float nextFMOscillatorSample(FMOperator *op) {
    if (!op || !op->carrier || !op->modulator || !op->modEnvelope)
        return 0.0f;

    float mod_env_val = nextEnvelopeSample(op->modEnvelope);
    float mod_signal  = nextOscillatorSample(op->modulator) * op->modDepth * mod_env_val;

    float original_freq  = op->base_frequency;
    float modulated_freq = original_freq + mod_signal * op->modIndex * original_freq;

    setOscFrequency(op->carrier, modulated_freq);

    return nextOscillatorSample(op->carrier);
}

void FMOpSetModIndex(FMOperator *op, float index) {
    if (!op)
        return;
    op->modIndex = index;
}

void FMOpSetModDepth(FMOperator *op, float depth) {
    if (!op)
        return;
    op->modDepth = depth;
}
