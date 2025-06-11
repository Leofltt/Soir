#include "sequencer.h"

#include <3ds.h>

void updateSeqLength(Sequencer *seq, size_t newLength) { // Changed from int
    if (!seq || newLength == 0 || newLength > MAXSEQUENCELENGTH || newLength == seq->n_steps) {
        return;
    }

    SeqStep *old_steps = (SeqStep *) linearAlloc(seq->n_steps * sizeof(SeqStep));
    if (!old_steps) {
        return;
    }

    // Backup current steps
    for (size_t i = 0; i < seq->n_steps; i++) { // Changed from int
        old_steps[i] = seq->steps[i];
    }

    // Allocate new buffer
    SeqStep *new_steps = (SeqStep *) linearAlloc(newLength * sizeof(SeqStep));
    if (!new_steps) {
        linearFree(old_steps);
        return;
    }

    // Copy and extend if needed
    for (size_t i = 0; i < newLength; i++) { // Changed from int
        new_steps[i] = (i < seq->n_steps) ? old_steps[i] : old_steps[seq->n_steps - 1];
    }

    linearFree(old_steps);
    linearFree(seq->steps);
    seq->steps   = new_steps;
    seq->n_steps = newLength;
    seq->cur_step %= newLength;
}

void setSteps(Sequencer *seq, SeqStep *steps) {
    if (!seq || !steps)
        return;
    for (size_t i = 0; i < seq->n_steps; i++) { // Changed from int
        seq->steps[i] = steps[i];
    }
}

SeqStep updateSequencer(Sequencer *seq) {
    if (!seq || !seq->steps) {
        return (SeqStep) { .active = false, .data = NULL };
    }

    seq->cur_step = (seq->cur_step + 1) % seq->n_steps;
    return seq->steps[seq->cur_step];
}

void cleanupSequencer(Sequencer *seq) {
    if (!seq)
        return;

    if (seq->steps) {
        for (size_t i = 0; i < seq->n_steps; i++) {
            if (seq->steps[i].data) {
                // Note: don't free data here as it's owned by trackParamsArray
                seq->steps[i].data = NULL;
            }
        }
        linearFree(seq->steps);
        seq->steps = NULL;
    }

    seq->n_steps  = 0;
    seq->cur_step = 0;
    seq->n_beats  = 0;
}