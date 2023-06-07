#ifndef SYNTH_H
#define SYNTH_H

#include "oscillators.h"
#include "envelope.h"
#include "audio_utils.h"

typedef struct {
    Envelope* env;
    PolyBLEPOscillator* osc;
} SubSynth;

void fillSubSynthAudiobuffer(void* audioBuffer, size_t size, SubSynth* subsynth, float synthvol) {
    u32* dest = (u32*) audioBuffer;

	for (int i = 0; i < size; i++) {
        float next_sam = nextOscillatorSample(subsynth->osc);
        float env_value = nextEnvelopeSample(subsynth->env);
        next_sam *= env_value;
        next_sam *= synthvol;
        s16 sample = float_to_int16(next_sam);
        dest[i] = (sample << 16) | (sample & 0xffff);
    }

    DSP_FlushDataCache(audioBuffer, size);
};

#endif // SYNTH_H