#ifndef SAMPLERS_H
#define SAMPLERS_H

#include "envelope.h"
#include "sample.h"

#ifdef TESTING
#include "../tests/mock_3ds.h"
#else
#include <3ds/ndsp/ndsp.h>
#include <3ds/types.h>
#include <opusfile.h>
#endif

typedef enum { ONE_SHOT = 0, LOOP = 1 } PlaybackMode;

typedef struct {
    Sample      *sample;
    PlaybackMode playback_mode;
    int64_t      start_position;
    size_t       samples_per_buf;
    float        samplerate;
    Envelope    *env;
    size_t       current_frame;
    bool         finished;
} Sampler;

bool samplerIsLooping(Sampler *sampler);

void fillSamplerAudioBuffer(ndspWaveBuf *waveBuf_, size_t size, Sampler *sampler);

#endif // SAMPLERS_H