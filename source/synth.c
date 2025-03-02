#include "synth.h"

#include <3ds.h>

void fillSubSynthAudiobuffer(void* audioBuffer, size_t size, SubSynth* subsynth, float synthvol,
                             int chan_id) {
    u32* dest = (u32*)audioBuffer;

    for (int i = 0; i < size; i++) {
        float next_sam = nextOscillatorSample(subsynth->osc);
        float env_value = nextEnvelopeSample(subsynth->env);
        next_sam *= env_value;
        next_sam *= synthvol;
        s16 sample = float_to_int16(next_sam);
        dest[i] = (sample << 16) | (sample & 0xffff);
    }

    ndspChnWaveBufAdd(chan_id, audioBuffer);
    DSP_FlushDataCache(audioBuffer, size);
};