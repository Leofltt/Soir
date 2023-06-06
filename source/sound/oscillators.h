#ifndef OSCILLATORS_H
#define OSCILLATORS_H

#include <math.h>

#include "audio_utils.h"

typedef enum {
    SINE = 0,
    SQUARE = 1,
    SAW = 2    
} Waveform;

typedef struct {
    float frequency;
    float samplerate;
    Waveform waveform;
    float phase_inc;
    float phase;
} PolyBLEPOscillator;

void setOscFrequency(PolyBLEPOscillator* osc, float frequency) {
    osc->frequency = frequency;
    osc->phase_inc = osc->frequency * M_TWOPI / osc->samplerate;
};

float oscillate(PolyBLEPOscillator* osc) {
    float sample;
    switch (osc->waveform) {
        default:
            sample = sin(osc->phase);
            break;
        case SINE:
            sample = sin(osc->phase);
            break;
        case SQUARE:
            if (osc->phase < M_TWOPI) {
                sample = 1.0;
            } else {
                sample = -1.0;
            }
            break;
        case SAW:
           sample = (2.0f * osc->phase / M_TWOPI) - 1.0f;
           break;
    }
    return sample;
};

float polyBLEP(PolyBLEPOscillator* osc, float t) {
    float dt = osc->phase_inc / M_TWOPI;
    // 0 <= t < 1 : beginning of sample period
    if (t < dt) {
        t /= dt;
        return t+t - t*t - 1.0;
    }
    // -1 < t < 0 : right before the end of a sample period
    else if (t > 1.0 - dt) {
        t = (t - 1.0) / dt;
        return t*t + t+t + 1.0;
    }
    // 0 otherwise
    else return 0.0;
};


float nextOscillatorSample(PolyBLEPOscillator* osc) {
    float sample;
    float t = osc->phase_inc / M_TWOPI;
    
    if (osc->waveform == SINE) {
        sample = oscillate(osc);
    }
    else if (osc->waveform == SAW) {
        sample = oscillate(osc);
        sample -= polyBLEP(osc, t);
    }
    else { // SQUARE WAVE 
        sample = oscillate(osc);
        sample += polyBLEP(osc, t);
        sample -= polyBLEP(osc, fmod(t + 0.5, 1.0));
        // if (osc.waveform == TRIANGLE)
        // {
        //     // Leaky integrator: y[n] = A * x[n] + (1 - A) * y[n-1]
        //     value = mPhaseInc * value + (1 - mPhaseInc) * lastoutput;
        //     lastoutput = value;
        // }
    }

    osc->phase += osc->phase_inc;
    while (osc->phase >= M_TWOPI) {
        osc->phase -= M_TWOPI;
    }
    
    return sample;
};

void fillPolyblepAudiobuffer(void* audioBuffer, size_t size, PolyBLEPOscillator* osc) {
	u32* dest = (u32*) audioBuffer;

	for (int i = 0; i < size; i++) {
        float next_sam = nextOscillatorSample(osc);
		s16 sample = float_to_int16(next_sam);

		dest[i] = (sample << 16) | (sample & 0xffff);
	}

	DSP_FlushDataCache(audioBuffer, size);
}

#endif // OSCILLATORS_H