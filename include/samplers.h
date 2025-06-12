#ifndef SAMPLERS_H
#define SAMPLERS_H

#include "envelope.h"

#ifdef TESTING
#include "../tests/mock_3ds.h"
#else
#include <3ds/types.h>
#include <opusfile.h>
#include <3ds/ndsp/ndsp.h>
#endif

typedef enum { ONE_SHOT = 0, LOOP = 1 } PlaybackMode;

typedef struct {
    OggOpusFile *audiofile;
    PlaybackMode playback_mode;
    int64_t      start_position;
    size_t       samples_per_buf;
    float        samplerate;
    Envelope    *env;
} OpusSampler;

extern const char *opusStrError(int error);

extern void setSample(OpusSampler *sampler, char *path);

extern bool isLooping(OpusSampler *sampler);

extern void fillSamplerAudiobuffer(ndspWaveBuf *waveBuf_, size_t size, OpusSampler *sampler,
                                   int chan_id);

#endif // SAMPLERS_H