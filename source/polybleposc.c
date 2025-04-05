#include "polybleposc.h"

const char *waveform_names[] = { "Sine", "Square", "Saw", "Triangle" };

void setWaveform(PolyBLEPOscillator *osc, int wf_idx) {
    osc->waveform = (Waveform) wf_idx;
};

void setOscFrequency(PolyBLEPOscillator *osc, float frequency) {
    osc->frequency = frequency;
    osc->phase_inc = frequency * M_TWOPI / osc->samplerate;
};

float nextOscillatorSample(PolyBLEPOscillator *osc) {
    float sample;
    float t = osc->phase / M_TWOPI;

    switch (osc->waveform) {
    case SINE:
        sample = sin(osc->phase);
        break;
    case SQUARE:
        float square     = t < 0.5f ? 1.0f : -1.0f;
        float squarePoly = polyBLEP(osc, t);
        sample           = square + squarePoly - polyBLEP(osc, fmod(t + 0.5f, 1.0f));
        break;
    case SAW:
        sample = ((2.0f * t) - 1.0f) - polyBLEP(osc, t);
        break;
    case TRIANGLE:
        if (t < 0.5f) {
            sample = 4.0f * t - 1.0f;
        } else {
            sample = -4.0f * (t - 0.5f) + 1.0f;
        }
        break;
    default:
        sample = sin(osc->phase);
        break;
    }
    osc->phase += osc->phase_inc;
    while (osc->phase >= M_TWOPI) {
        osc->phase -= M_TWOPI;
    }
    return sample;
};

float polyBLEP(PolyBLEPOscillator *osc, float t) {
    const float dt = osc->phase_inc / M_TWOPI;

    // Calculate the polyblep value based on the phase within one period
    if (t < 0 || t > 1) {
        return 0.0f;
    } else if (t < dt) {
        // At the beginning of the sample period
        t /= dt;
        return t + t - t * t - 1.0f;
    } else if (t > 1 - dt) {
        // At the end of the sample period
        t = (t - 1.0f) / dt;
        return t * t + t + t + 1.0f;
    }

    // If phase is within a valid range, polyblep value is zero
    return 0.0f;
};