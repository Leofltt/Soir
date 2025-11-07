#ifdef TESTING
#include "../tests/mock_3ds.h"
#else
#include <3ds/types.h>
#include <3ds/allocator/linear.h>
#include <3ds/ndsp/ndsp.h>
#include <3ds/ndsp/channel.h> // Added for ndspChnWaveBufAdd
#include <3ds/services/dsp.h>
#endif

#include "samplers.h"
#include "sample.h"

#include "audio_utils.h"
#include "engine_constants.h"

#include <string.h>

bool samplerIsLooping(Sampler *sampler) {
    return sampler->playback_mode == LOOP;
}

void fillSamplerAudioBuffer(ndspWaveBuf *waveBuf_, size_t size, Sampler *sampler) {
#ifdef DEBUG
    // Setup timer for performance stats
    TickCounter timer;
    osTickCounterStart(&timer);
#endif // DEBUG

    if (!sampler->sample || !sampler->sample->pcm_data) {
        memset(waveBuf_->data_pcm16, 0, sampler->samples_per_buf * NCHANNELS * sizeof(int16_t));
        waveBuf_->nsamples = sampler->samples_per_buf;
        DSP_FlushDataCache(waveBuf_->data_pcm16,
                           sampler->samples_per_buf * NCHANNELS * sizeof(int16_t));
        return;
    }

    if (sampler->playback_mode == ONE_SHOT && sampler->finished) {
        memset(waveBuf_->data_pcm16, 0, sampler->samples_per_buf * NCHANNELS * sizeof(int16_t));
        waveBuf_->nsamples = sampler->samples_per_buf;
        DSP_FlushDataCache(waveBuf_->data_pcm16,
                           sampler->samples_per_buf * NCHANNELS * sizeof(int16_t));
        return;
    }

    int totalSamples = 0;
    while (totalSamples < sampler->samples_per_buf) {
        size_t remaining_samples_in_buffer = sampler->samples_per_buf - totalSamples;
        size_t remaining_samples_in_sample =
            sampler->sample->pcm_data_size_in_frames - sampler->current_frame;
        size_t samples_to_copy = remaining_samples_in_buffer < remaining_samples_in_sample
                                     ? remaining_samples_in_buffer
                                     : remaining_samples_in_sample;

        if (samples_to_copy > 0) {
            memcpy(waveBuf_->data_pcm16 + totalSamples * NCHANNELS,
                   sampler->sample->pcm_data + sampler->current_frame * NCHANNELS,
                   samples_to_copy * NCHANNELS * sizeof(int16_t));
            sampler->current_frame += samples_to_copy;
            totalSamples += samples_to_copy;
        }

        if (sampler->current_frame >= sampler->sample->pcm_data_size_in_frames) {
            if (samplerIsLooping(sampler)) {
                sampler->current_frame = 0;
            } else {
                sampler->finished = true;
                break;
            }
        }
    }

    if (totalSamples < sampler->samples_per_buf) {
        memset(waveBuf_->data_pcm16 + totalSamples * NCHANNELS, 0,
               (sampler->samples_per_buf - totalSamples) * NCHANNELS * sizeof(int16_t));
    }

    for (int i = 0; i < totalSamples; i++) {
        float env_value = nextEnvelopeSample(sampler->env);
        env_value       = fmaxf(0.0f, fminf(1.0f, env_value));

        int sample_idx = i * NCHANNELS;

        float left_sam  = int16ToFloat(waveBuf_->data_pcm16[sample_idx]);
        float right_sam = int16ToFloat(waveBuf_->data_pcm16[sample_idx + 1]);

        left_sam *= env_value;
        right_sam *= env_value;

        waveBuf_->data_pcm16[sample_idx]     = floatToInt16(left_sam);
        waveBuf_->data_pcm16[sample_idx + 1] = floatToInt16(right_sam);
    }

    waveBuf_->nsamples = sampler->samples_per_buf;
    DSP_FlushDataCache(waveBuf_->data_pcm16,
                       sampler->samples_per_buf * NCHANNELS * sizeof(int16_t));

#ifdef DEBUG
    // Print timing info
    osTickCounterUpdate(&timer);

    osTickCounterRead(&timer);
#endif // DEBUG

    return;
};
