#ifndef SAMPLERS_H
#define SAMPLERS_H

#include "envelope.h"
#include "track_parameters.h"
#include <opusfile.h>

#ifndef TESTING
#include <3ds.h>
#endif

typedef struct OpusSampler {
    OggOpusFile *audiofile;
    const char *path;
    Envelope    *env;
    int64_t      start_position;
    size_t       samples_per_buf;
    float        samplerate;
    PlaybackMode playback_mode;
    bool         seek_requested;
    bool         finished;
} OpusSampler;

struct Track;

extern void fillSamplerAudiobuffer(struct Track *track, ndspWaveBuf *waveBuf, size_t size);

#endif // SAMPLERS_H