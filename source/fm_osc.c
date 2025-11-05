#include "fm_osc.h"
#include "polybleposc.h"
#include <math.h>

void setFMOscFrequency(FMOperator *osc, float frequency) {
    if (!osc || !osc->carrier || !osc->modulator)
        return;
    setOscFrequency(osc->carrier, frequency);
    // For now, let's assume a simple 1:1 ratio for the modulator
    setOscFrequency(osc->modulator, frequency);
}

float nextFMOscillatorSample(FMOperator *osc) {
    if (!osc || !osc->carrier || !osc->modulator || !osc->modEnvelope)
        return 0.0f;

    float mod_env_val = nextEnvelopeSample(osc->modEnvelope);
    float mod_signal  = nextOscillatorSample(osc->modulator) * osc->modDepth * mod_env_val;

    float original_freq  = osc->carrier->phase_inc * osc->carrier->samplerate / (2.0f * M_PI);
    float modulated_freq = original_freq + mod_signal * osc->modIndex * original_freq;

    setOscFrequency(osc->carrier, modulated_freq);

    return nextOscillatorSample(osc->carrier);
}

void setModIndex(FMOperator *osc, float index) {
    if (!osc)
        return;
    osc->modIndex = index;
}

void setModDepth(FMOperator *osc, float depth) {
    if (!osc)
        return;
    osc->modDepth = depth;
}