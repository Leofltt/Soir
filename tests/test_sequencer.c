#include "mock_3ds.h"
#include "sequencer.h"
#include "unity.h"

void test_sequence_length_update(void) {
    Sequencer seq = { .n_beats = 4, .steps_per_beat = 4 };
    seq.steps     = linearAlloc((seq.n_beats * seq.steps_per_beat) * sizeof(SeqStep));

    updateSeqLength(&seq, 8);
    TEST_ASSERT_EQUAL(2, seq.n_beats);

    cleanupSequencer(&seq);
}

void test_sequence_step_update(void) {
    Sequencer seq = { .n_beats = 1, .steps_per_beat = 4, .cur_step = 0 };
    seq.steps     = linearAlloc((seq.n_beats * seq.steps_per_beat) * sizeof(SeqStep));

    for (size_t i = 0; i < 4; i++) {
        seq.steps[i].active = (i % 2 == 0);
    }

    SeqStep step = updateSequencer(&seq);
    TEST_ASSERT_TRUE(step.active);
    TEST_ASSERT_EQUAL(1, seq.cur_step);
    step = updateSequencer(&seq);
    TEST_ASSERT_FALSE(step.active);
    TEST_ASSERT_EQUAL(2, seq.cur_step);

    cleanupSequencer(&seq);
}
