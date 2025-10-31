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

bool sampler_is_looping(Sampler *sampler) {
    return sampler->playback_mode == LOOP;
}

void sampler_fill_buffer(ndspWaveBuf *waveBuf_, size_t size, Sampler *sampler) {
#ifdef DEBUG
    // Setup timer for performance stats
    TickCounter timer;
    osTickCounterStart(&timer);
#endif // DEBUG

    if (!sampler->sample) {
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

    // Decode samples until our waveBuf is full
    int totalSamples = 0;
    while (totalSamples < sampler->samples_per_buf) {
        int16_t     *buffer     = waveBuf_->data_pcm16 + (totalSamples * NCHANNELS);
        const size_t bufferSize = (sampler->samples_per_buf - totalSamples);

        // Decode bufferSize samples from opusFile_ into buffer,
        // storing the number of samples that were decoded (or error)
        const int samples = op_read_stereo(sampler->sample->opusFile, buffer, bufferSize);
        if (samples <= 0) {
            if (sampler_is_looping(sampler)) {
                op_raw_seek(sampler->sample->opusFile, 0);
                continue;
            } else {
                sampler->finished = true;
                op_raw_seek(sampler->sample->opusFile,
                            sampler->start_position); // Reset for next trigger
                if (totalSamples < sampler->samples_per_buf) {
                    memset(waveBuf_->data_pcm16 + (totalSamples * NCHANNELS), 0,
                           (sampler->samples_per_buf - totalSamples) * NCHANNELS * sizeof(int16_t));
                }
                totalSamples = sampler->samples_per_buf;
                break;
            }
        }

        for (int i = 0; i < samples; i++) {
            float env_value = nextEnvelopeSample(sampler->env);
            env_value       = fmaxf(0.0f, fminf(1.0f, env_value));

            int sample_idx = (totalSamples + i) * NCHANNELS;

            float left_sam  = int16ToFloat(waveBuf_->data_pcm16[sample_idx]);
            float right_sam = int16ToFloat(waveBuf_->data_pcm16[sample_idx + 1]);

            left_sam *= env_value;
            right_sam *= env_value;

            waveBuf_->data_pcm16[sample_idx]     = floatToInt16(left_sam);
            waveBuf_->data_pcm16[sample_idx + 1] = floatToInt16(right_sam);
        }

        totalSamples += samples;
    }

    // If no samples were read in the last decode cycle and looping is on,
    // seek back to the start of the sample
    if (totalSamples == 0 && sampler_is_looping(sampler)) {
        op_raw_seek(sampler->sample->opusFile, 0);
    }

    // Pass samples to NDSP
    waveBuf_->nsamples = totalSamples;
    DSP_FlushDataCache(waveBuf_->data_pcm16, totalSamples * NCHANNELS * sizeof(int16_t));

#ifdef DEBUG
    // Print timing info
    osTickCounterUpdate(&timer);
    printf("fillBuffer %lfms in %lfms\n", totalSamples * 1000.0 / SAMPLE_RATE,
           osTickCounterRead(&timer));
#endif // DEBUG

    return;
};
