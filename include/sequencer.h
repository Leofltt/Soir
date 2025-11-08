#ifndef SEQUENCER_H
#define SEQUENCER_H

#ifdef TESTING
#include "../tests/mock_3ds.h"
#else
#include <3ds/types.h>
#endif

#include "clock.h"
#include "track_parameters.h"

#define MAXSEQUENCELENGTH                                                                          \
    (STEPS_PER_BEAT * MAXSUBDIVBEAT) // 384, should be more than enough for most use cases

typedef struct {
    bool             active;
    TrackParameters *data;
} SeqStep;

typedef struct {
    int              n_beats;
    int              steps_per_beat;
    size_t           cur_step;
    SeqStep         *steps;
    void            *instrument_params_array;
    TrackParameters *track_params_array;
} Sequencer;

extern void    updateSeqLength(Sequencer *seq, size_t newLength);
extern SeqStep updateSequencer(Sequencer *seq);
extern void    setSteps(Sequencer *seq, SeqStep *steps);
extern void    cleanupSequencer(Sequencer *seq);

#endif // SEQUENCER_H