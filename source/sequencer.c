#ifdef TESTING
#include "../tests/mock_3ds.h"
#else
#include <3ds/types.h>
#include <3ds/allocator/linear.h> // Updated path
#endif

#include "sequencer.h"

void updateSeqLength(Sequencer *seq, size_t newLength) { // Changed from int
    if (!seq || newLength == 0 || newLength > MAXSEQUENCELENGTH || newLength == (seq->n_beats * seq->steps_per_beat)) {
        return;
    }

    size_t old_n_steps = seq->n_beats * seq->steps_per_beat;
    SeqStep *old_steps = (SeqStep *) linearAlloc(old_n_steps * sizeof(SeqStep));
    if (!old_steps) {
        return;
    }

    // Backup current steps
    for (size_t i = 0; i < old_n_steps; i++) { // Changed from int
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
        new_steps[i] = (i < old_n_steps) ? old_steps[i] : old_steps[old_n_steps - 1];
    }

    linearFree(old_steps);
    linearFree(seq->steps);
    seq->steps   = new_steps;
    seq->n_beats = newLength / seq->steps_per_beat;
    seq->cur_step %= newLength;
}

void setSteps(Sequencer *seq, SeqStep *steps) {
    if (!seq || !steps)
        return;
    for (size_t i = 0; i < (seq->n_beats * seq->steps_per_beat); i++) { // Changed from int
        seq->steps[i] = steps[i];
    }
}

SeqStep updateSequencer(Sequencer *seq) {
    if (!seq || !seq->steps) {
        return (SeqStep) { .active = false, .data = NULL };
    }

    SeqStep current_step = seq->steps[seq->cur_step];
    seq->cur_step        = (seq->cur_step + 1) % (seq->n_beats * seq->steps_per_beat);
    return current_step;
}

void cleanupSequencer(Sequencer *seq) {
    if (!seq)
        return;

    if (seq->steps) {
        for (size_t i = 0; i < (seq->n_beats * seq->steps_per_beat); i++) {
            if (seq->steps[i].data) {
                // Note: don't free data here as it's owned by trackParamsArray
                seq->steps[i].data = NULL;
            }
        }
        linearFree(seq->steps);
        seq->steps = NULL;
    }

    seq->cur_step = 0;
    seq->n_beats  = 0;
}