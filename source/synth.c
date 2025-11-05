#ifdef TESTING
#include "../tests/mock_3ds.h"
#else
#include <3ds/ndsp/ndsp.h>
#include <3ds/services/dsp.h>
#include <3ds/types.h>
#endif

#include "audio_utils.h"
#include "synth.h"
#include "engine_constants.h"

void fillFMSynthAudiobuffer(ndspWaveBuf *waveBuf, size_t size, FMSynth *fm_synth) {
    if (!fm_synth || !fm_synth->fm_op || !fm_synth->carrierEnv) {
        fillBufferWithZeros(waveBuf->data_pcm16, size * NCHANNELS * sizeof(int16_t));
        return;
    }

    for (size_t i = 0; i < size; i++) {
        float sample  = nextFMOscillatorSample(fm_synth->fm_op);
        float env_val = nextEnvelopeSample(fm_synth->carrierEnv);
        sample *= env_val;

        int16_t sample_i16                     = floatToInt16(sample);
        waveBuf->data_pcm16[i * NCHANNELS]     = sample_i16;
        waveBuf->data_pcm16[i * NCHANNELS + 1] = sample_i16;
    }
    waveBuf->nsamples = size;
    DSP_FlushDataCache(waveBuf->data_pcm16, size * NCHANNELS * sizeof(int16_t));
}

void fillSubSynthAudiobuffer(ndspWaveBuf *waveBuf, size_t size, SubSynth *subsynth) {
    u32 *dest = (u32 *) waveBuf->data_pcm16;

    for (int i = 0; i < size; i++) {
        float next_sam  = nextOscillatorSample(subsynth->osc);
        next_sam        = fmaxf(-1.0f, fminf(1.0f, next_sam));
        float env_value = nextEnvelopeSample(subsynth->env);
        env_value       = fmaxf(0.0f, fminf(1.0f, env_value));
        next_sam *= env_value;
        s16 sample = floatToInt16(next_sam);
        dest[i]    = (sample << 16) | (sample & 0xffff);
    }

    DSP_FlushDataCache(waveBuf->data_pcm16, size);
};