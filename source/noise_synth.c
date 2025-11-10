#include "noise_synth.h"
#include "audio_utils.h"
#include "engine_constants.h"
#include <3ds/services/dsp.h>
#include <string.h>

void fillNoiseSynthAudiobuffer(ndspWaveBuf *waveBuf, size_t size, NoiseSynth *noiseSynth) {
    if (!noiseSynth || !noiseSynth->env) {
        memset(waveBuf->data_pcm16, 0, size * NCHANNELS * sizeof(int16_t));
        waveBuf->nsamples = size;
        DSP_FlushDataCache(waveBuf->data_pcm16, size * NCHANNELS * sizeof(int16_t));
        return;
    }

    for (size_t i = 0; i < size; i++) {
        // 15-bit LFSR for GBA-style noise
        unsigned int bit =
            ((noiseSynth->lfsr_register >> 0) ^ (noiseSynth->lfsr_register >> 1)) & 1;
        noiseSynth->lfsr_register = (noiseSynth->lfsr_register >> 1) | (bit << 14);

        // Use the lowest bit as the audio output
        float sample = (noiseSynth->lfsr_register & 1) ? 1.0f : -1.0f;

        sample *= nextEnvelopeSample(noiseSynth->env);

        int16_t sample_i16 = floatToInt16(sample);

        waveBuf->data_pcm16[i * NCHANNELS]     = sample_i16;
        waveBuf->data_pcm16[i * NCHANNELS + 1] = sample_i16; // Mono
    }

    waveBuf->nsamples = size;
    DSP_FlushDataCache(waveBuf->data_pcm16, size * NCHANNELS * sizeof(int16_t));
}
