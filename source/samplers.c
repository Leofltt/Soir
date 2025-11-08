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
    return sampler->playback_mode == LOOP && sampler->sample &&
           sampler->sample->pcm_data_size_in_frames > 0;
}

void fillSamplerAudioBuffer(ndspWaveBuf *waveBuf_, size_t size, Sampler *sampler) {
    if (!sampler->sample || !sampler->sample->pcm_data) {
        memset(waveBuf_->data_pcm16, 0, sampler->samples_per_buf * NCHANNELS * sizeof(int16_t));
        waveBuf_->nsamples = sampler->samples_per_buf;
        DSP_FlushDataCache(waveBuf_->data_pcm16,
                           sampler->samples_per_buf * NCHANNELS * sizeof(int16_t));
        return;
    }

    int totalSamples = 0;
    while (totalSamples < sampler->samples_per_buf) {
        float   env_value = nextEnvelopeSample(sampler->env);
        int16_t left_s    = 0;
        int16_t right_s   = 0;

        if (!sampler->finished) {
            float left_f =
                int16ToFloat(sampler->sample->pcm_data[sampler->current_frame * NCHANNELS]);
            float right_f =
                int16ToFloat(sampler->sample->pcm_data[sampler->current_frame * NCHANNELS + 1]);

            left_f *= env_value;
            right_f *= env_value;

            left_s  = floatToInt16(left_f);
            right_s = floatToInt16(right_f);

            sampler->current_frame++;
            if (sampler->current_frame >= sampler->sample->pcm_data_size_in_frames) {
                if (samplerIsLooping(sampler)) {
                    sampler->current_frame = 0;
                } else {
                    sampler->finished = true;
                }
            }
        }

        waveBuf_->data_pcm16[totalSamples * NCHANNELS]     = left_s;
        waveBuf_->data_pcm16[totalSamples * NCHANNELS + 1] = right_s;
        totalSamples++;
    }

    waveBuf_->nsamples = sampler->samples_per_buf;
    DSP_FlushDataCache(waveBuf_->data_pcm16,
                       sampler->samples_per_buf * NCHANNELS * sizeof(int16_t));
};
