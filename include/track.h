#ifndef TRACK_H
#define TRACK_H

#include "filters.h"

#include <3ds/ndsp/ndsp.h>
#include <3ds/types.h>

typedef enum { SUB_SYNTH = 0, OPUS_SAMPLER = 1, FM_SYNTH = 2 } InstrumentType;

typedef struct {
    int            chan_id;
    float         *mix; // mix buffer for the channel (each channel has an array of 12 floats)
    InstrumentType instrument_type;
    ndspWaveBuf    waveBuf[2];
    u32           *audioBuffer;
    NdspBiquad    *filter;
    bool           is_muted;
    bool           is_soloed;

} Track;

extern void initializeTrack(Track *track);
extern void resetTrack(Track *track);

#endif // TRACK_H