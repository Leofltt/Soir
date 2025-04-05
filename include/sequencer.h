#ifndef SEQUENCER_H
#define SEQUENCER_H

#include "clock.h"

#define MAXSEQUENCELENGTH \
    (STEPS_PER_BEAT * MAXSUBDIVBEAT)  // 384, should be more than enough for most use cases

typedef struct {
    bool active;
    void *data;
} SeqStep;

typedef struct {
    // int chan_id;
    int n_steps;
    int n_beats;
    int cur_step;
    SeqStep* steps;
} Sequencer;

extern void updateSeqLength(Sequencer* seq, int newLength);
extern SeqStep updateSequencer(Sequencer* seq);
extern void setSteps(Sequencer* seq, SeqStep* steps);

#endif  // SEQUENCER_H