#include "samplers.h"

#include "audio_utils.h"
#include "engine_constants.h"

#include <string.h>

const char *opusStrError(int error) {
    switch (error) {
    case OP_FALSE:
        return "OP_FALSE: A request did not succeed.";
    case OP_HOLE:
        return "OP_HOLE: There was a hole in the page sequence numbers.";
    case OP_EREAD:
        return "OP_EREAD: An underlying read, seek or tell operation "
               "failed.";
    case OP_EFAULT:
        return "OP_EFAULT: A NULL pointer was passed where none was "
               "expected, or an internal library error was encountered.";
    case OP_EIMPL:
        return "OP_EIMPL: The stream used a feature which is not "
               "implemented.";
    case OP_EINVAL:
        return "OP_EINVAL: One or more parameters to a function were "
               "invalid.";
    case OP_ENOTFORMAT:
        return "OP_ENOTFORMAT: This is not a valid Ogg Opus stream.";
    case OP_EBADHEADER:
        return "OP_EBADHEADER: A required header packet was not properly "
               "formatted.";
    case OP_EVERSION:
        return "OP_EVERSION: The ID header contained an unrecognised "
               "version number.";
    case OP_EBADPACKET:
        return "OP_EBADPACKET: An audio packet failed to decode properly.";
    case OP_EBADLINK:
        return "OP_EBADLINK: We failed to find data we had seen before or "
               "the stream was sufficiently corrupt that seeking is "
               "impossible.";
    case OP_ENOSEEK:
        return "OP_ENOSEEK: An operation that requires seeking was "
               "requested on an unseekable stream.";
    case OP_EBADTIMESTAMP:
        return "OP_EBADTIMESTAMP: The first or last granule position of a "
               "link failed basic validity checks.";
    default:
        return "Unknown error.";
    }
};

void setSample(OpusSampler *sampler, char *path) {
    int error = 0;
    if (sampler->audiofile) {
        op_free(sampler->audiofile);
    }
    sampler->audiofile = op_open_file(path, &error);
};

bool isLooping(OpusSampler *sampler) {
    return sampler->playback_mode == LOOP;
}

void fillSamplerAudiobuffer(ndspWaveBuf *waveBuf_, size_t size, OpusSampler *sampler, int chan_id) {
#ifdef DEBUG
    // Setup timer for performance stats
    TickCounter timer;
    osTickCounterStart(&timer);
#endif // DEBUG

    // Decode samples until our waveBuf is full
    int totalSamples = 0;
    while (totalSamples < sampler->samples_per_buf) {
        int16_t     *buffer     = waveBuf_->data_pcm16 + (totalSamples * NCHANNELS);
        const size_t bufferSize = (sampler->samples_per_buf - totalSamples) * NCHANNELS;

        // Decode bufferSize samples from opusFile_ into buffer,
        // storing the number of samples that were decoded (or error)
        const int samples = op_read_stereo(sampler->audiofile, buffer, bufferSize);
        if (samples <= 0) {
            if (!isLooping(sampler)) {
                // If loop is off and no more samples are available, fill buffers with zeros
                for (size_t i = totalSamples; i < sampler->samples_per_buf; ++i) {
                    int16_t *bufferPtr = waveBuf_->data_pcm16 + (i * NCHANNELS);
                    memset(bufferPtr, 0, NCHANNELS * sizeof(int16_t));
                }
            } else {
                op_raw_seek(sampler->audiofile, 0);
            }
            return;
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
    if (totalSamples == 0 && isLooping(sampler)) {
        op_raw_seek(sampler->audiofile, 0);
    }

    // Pass samples to NDSP
    waveBuf_->nsamples = totalSamples;
    DSP_FlushDataCache(waveBuf_->data_pcm16, totalSamples * NCHANNELS * sizeof(int16_t));
    ndspChnWaveBufAdd(chan_id, waveBuf_);

#ifdef DEBUG
    // Print timing info
    osTickCounterUpdate(&timer);
    printf("fillBuffer %lfms in %lfms\n", totalSamples * 1000.0 / SAMPLE_RATE,
           osTickCounterRead(&timer));
#endif // DEBUG

    return;
};