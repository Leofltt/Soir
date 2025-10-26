#ifndef TRACK_H
#define TRACK_H

#include "clock.h"
#include "filters.h"
#include "event_queue.h"
#include "sequencer.h"
#include "synth.h"
#include "ui_constants.h"
#include "track_parameters.h"

#ifndef TESTING
#include <3ds.h>
#endif

typedef enum { SUB_SYNTH = 0, OPUS_SAMPLER = 1, FM_SYNTH = 2 } InstrumentType;

typedef struct Track {
    int            chan_id;
    float          mix[12];
    InstrumentType instrument_type;
    void          *instrument_data;
    ndspWaveBuf    waveBuf[2];
    u32           *audioBuffer;
    NdspBiquad     filter;
    bool           is_muted;
    bool           is_soloed;
    bool           fillBlock;
    Sequencer     *sequencer;
    EventQueue     event_queue;

} Track;

extern void initializeTrack(Track *track, int chan_id, InstrumentType instrument_type, float rate,
                            u32 num_samples, u32 *audio_buffer);
extern void resetTrack(Track *track);
extern void updateTrack(Track *track, Clock *clock);
extern void updateTrackParameters(Track *track, TrackParameters *params);

#endif // TRACK_H