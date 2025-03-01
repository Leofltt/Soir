#ifndef SAMPLERS_H
#define SAMPLERS_H

#include <3ds.h>
#include <opusfile.h>

typedef enum {
    ONE_SHOT = 0,
    LOOP = 1
} PlaybackMode;

typedef struct {
    OggOpusFile* audiofile;
    float playback_speed;
    float samplerate;
    PlaybackMode playback_mode;
} OpusSampler;

extern const char *opusStrError(int error);

extern void setSample(OpusSampler* sampler, char* path);

extern void fillSamplerAudiobuffer(void* audioBuffer, size_t size, OpusSampler* sampler, int chan_id);

extern bool fillBuffer(OggOpusFile *opusFile_, ndspWaveBuf *waveBuf_, int chan_id);

#endif // SAMPLERS_H