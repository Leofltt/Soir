#include "samplers.h"
#include "audio_utils.h"

const char *opusStrError(int error) {
    switch(error) {
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

void setSample(OpusSampler* sampler, char* path) {
    int error = 0;
    OggOpusFile *opusFile = op_open_file(path, &error);
    if(error) {
        printf("Failed to open file: error %d (%s)\n", error,
               opusStrError(error));
        return;
    }
    sampler->audiofile = opusFile;
};

void fillSamplerAudiobuffer(void* audioBuffer, size_t size, OpusSampler* sampler, int chan_id) {
    // int totalSamples = 0;

    ndspChnWaveBufAdd(chan_id, audioBuffer);
    DSP_FlushDataCache(audioBuffer, size);
};

// This function pulls and decodes audio samples from opusFile_ to fill waveBuf_
bool fillBuffer(OggOpusFile *opusFile_, ndspWaveBuf *waveBuf_, int chan_id) {

    // Decode samples until our waveBuf is full
    int totalSamples = 0;
    while(totalSamples < SAMPLESPERBUF) {
        int16_t *buffer = waveBuf_->data_pcm16 + (totalSamples *
            NCHANNELS);
        const size_t bufferSize = (SAMPLESPERBUF - totalSamples) *
            NCHANNELS;

        // Decode bufferSize samples from opusFile_ into buffer,
        // storing the number of samples that were decoded (or error)
        const int samples = op_read_stereo(opusFile_, buffer, bufferSize);
        if(samples <= 0) {
            if(samples == 0) break;  // No error here

            printf("op_read_stereo: error %d (%s)", samples,
                   opusStrError(samples));
            break;
        }
        
        totalSamples += samples;
    }

    // If no samples were read in the last decode cycle, we're done
    if(totalSamples == 0) {
        // printf("Playback complete, press Start to exit\n");
        // return false;
    }

    // Pass samples to NDSP
    waveBuf_->nsamples = totalSamples;
    ndspChnWaveBufAdd(chan_id, waveBuf_);
    DSP_FlushDataCache(waveBuf_->data_pcm16,
        totalSamples * NCHANNELS * sizeof(int16_t));

    return true;
};