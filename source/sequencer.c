#include "sequencer.h"

#include <3ds.h>

void updateSeqLength(Sequencer *seq, int newLength) {
    if (newLength <= MAXSEQUENCELENGTH && newLength != seq->n_steps) {
        int      old_length = seq->n_steps;
        SeqStep *old_steps  = (SeqStep *) linearAlloc(seq->n_steps);
        for (int i = 0; i < seq->n_steps; i++) {
            old_steps[i] = seq->steps[i];
        }
        linearFree(seq->steps);
        seq->n_steps = newLength;
        seq->steps   = (SeqStep *) linearAlloc(newLength);
        for (int i = 0; i < newLength; i++) {
            if (i < old_length)
                seq->steps[i] = old_steps[i];
            else
                seq->steps[i] = old_steps[old_length - 1];
        }
        linearFree(old_steps);
        if (seq->cur_step >= newLength) {
            seq->cur_step = seq->cur_step % newLength;
        }
    }
};

void setSteps(Sequencer *seq, SeqStep *steps) {
    for (int i = 0; i <= seq->n_steps; i++) {
        seq->steps[i] = steps[i];
    }
}

SeqStep updateSequencer(Sequencer *seq) {
    seq->cur_step++;
    if (seq->cur_step >= seq->n_steps) {
        seq->cur_step = 0;
    }

    return (seq->steps[seq->cur_step]);
};