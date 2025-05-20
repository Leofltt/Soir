#ifndef TRACK_H
#define TRACK_H

#include "filters.h"

#include <3ds/types.h>
#include <3ds/ndsp/ndsp.h>

typedef enum { SYNTHESISER = 0, OPUS_SAMPLER = 1 } InstrumentType;

typedef struct {
    int            chan_id;
    float         *mix;
    InstrumentType instrument_type;
    ndspWaveBuf    waveBuf[2];
    u32           *audioBuffer;
    NdspBiquad    *filter;

} Track;

extern void initializeTrack(Track *track);
extern void resetTrack(Track *track);

#endif // TRACK_H