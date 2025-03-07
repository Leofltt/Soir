#ifndef SEQUENCER_H
#define SEQUENCER_H

#include "clock.h"

typedef struct {
    int chan_id;
    int n_steps;
    int cur_step;
    ClockStatus* seq_status;

} Sequencer;

extern void update_sequencer(Sequencer* sequencer);

#endif  // SEQUENCER_H