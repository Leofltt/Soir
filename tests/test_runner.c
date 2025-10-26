#include "unity.h"
#include "mock_3ds.h"

void setUp(void) {
    // Reset mocks before each test
    mock_set_system_tick(0);
}
void tearDown(void) {}

// Declare test functions
extern void test_sequence_length_update(void);
extern void test_sequence_step_update(void);
extern void test_envelope_initialization(void);

// Clock tests
extern void test_setBpm_calculates_correct_ticks_per_step(void);
extern void test_updateClock_should_not_tick_if_not_enough_time_has_passed(void);
extern void test_updateClock_should_tick_and_update_musical_time(void);
extern void test_updateClock_accumulator_handles_remainder(void);
extern void test_updateClock_does_not_tick_when_stopped(void);

// Event queue tests
extern void test_event_queue_init_should_set_head_and_tail_to_zero(void);
extern void test_event_queue_push_and_pop_should_work_correctly(void);
extern void test_event_queue_should_not_push_when_full(void);
extern void test_event_queue_should_not_pop_when_empty(void);
extern void test_event_queue_wraparound_should_work_correctly(void);

int main(void) {
    UNITY_BEGIN();

    // Sequencer tests
    RUN_TEST(test_sequence_length_update);
    RUN_TEST(test_sequence_step_update);

    // Envelope tests
    RUN_TEST(test_envelope_initialization);

    // Clock tests
    RUN_TEST(test_setBpm_calculates_correct_ticks_per_step);
    RUN_TEST(test_updateClock_should_not_tick_if_not_enough_time_has_passed);
    RUN_TEST(test_updateClock_should_tick_and_update_musical_time);
    RUN_TEST(test_updateClock_accumulator_handles_remainder);
    RUN_TEST(test_updateClock_does_not_tick_when_stopped);

    // Event queue tests
    RUN_TEST(test_event_queue_init_should_set_head_and_tail_to_zero);
    RUN_TEST(test_event_queue_push_and_pop_should_work_correctly);
    RUN_TEST(test_event_queue_should_not_push_when_full);
    RUN_TEST(test_event_queue_should_not_pop_when_empty);
    RUN_TEST(test_event_queue_wraparound_should_work_correctly);

    return UNITY_END();
}