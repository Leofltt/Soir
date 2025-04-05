#include "synth.h"

#include <3ds.h>

void fillSubSynthAudiobuffer(ndspWaveBuf *waveBuf, size_t size, SubSynth *subsynth, float synthvol,
                             int chan_id) {
    u32 *dest = (u32 *) waveBuf->data_pcm16;

    for (int i = 0; i < size; i++) {
        float next_sam  = nextOscillatorSample(subsynth->osc);
        next_sam        = fmaxf(-1.0f, fminf(1.0f, next_sam));
        float env_value = nextEnvelopeSample(subsynth->env);
        env_value       = fmaxf(0.0f, fminf(1.0f, env_value));
        next_sam *= env_value;
        next_sam *= synthvol;
        s16 sample = float_to_int16(next_sam);
        dest[i]    = (sample << 16) | (sample & 0xffff);
    }

    DSP_FlushDataCache(waveBuf->data_pcm16, size);
    ndspChnWaveBufAdd(chan_id, waveBuf);
};