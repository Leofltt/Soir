#include "unity.h"
#include "sequencer.h"
#include "mock_3ds.h"

void test_sequence_length_update(void) {
    Sequencer seq = { .n_steps = 16, .n_beats = 4 };
    seq.steps = linearAlloc(16 * sizeof(SeqStep));
    
    updateSeqLength(&seq, 8);
    TEST_ASSERT_EQUAL(8, seq.n_steps);
    
    cleanupSequencer(&seq);
}

void test_sequence_step_update(void) {
    Sequencer seq = { .n_steps = 4, .n_beats = 1, .cur_step = 0 };
    seq.steps = linearAlloc(4 * sizeof(SeqStep));
    
    for(size_t i = 0; i < 4; i++) {
        seq.steps[i].active = (i % 2 == 0);
    }
    
    SeqStep step = updateSequencer(&seq);
    TEST_ASSERT_FALSE(step.active);
    TEST_ASSERT_EQUAL(1, seq.cur_step);
    step = updateSequencer(&seq);
    TEST_ASSERT_TRUE(step.active);
    TEST_ASSERT_EQUAL(2, seq.cur_step);
    
    cleanupSequencer(&seq);
}