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
    size_t   n_steps;
    int      n_beats;
    size_t   cur_step;
    SeqStep *steps;
} Sequencer;

extern void    updateSeqLength(Sequencer *seq, size_t newLength);
extern SeqStep updateSequencer(Sequencer *seq);
extern void    setSteps(Sequencer *seq, SeqStep *steps);
extern void    cleanupSequencer(Sequencer *seq);

#endif // SEQUENCER_H