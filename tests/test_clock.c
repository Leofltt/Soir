#include "clock.h"
#include "mock_3ds.h"
#include "unity.h"
#include <math.h>

// This must match the value in clock.c
#define CLOCK_RESOLUTION_SHIFT 8

// Helper to get the precise unscaled ticks per step for tests
double get_unscaled_ticks_per_step_f(float bpm) {
    double ticks_per_beat = (double)SYSCLOCK_ARM11 * 60.0 / bpm;
    return ticks_per_beat / STEPS_PER_BEAT;
}

void test_setBpm_calculates_correct_ticks_per_step(void) {
    MusicalTime mt = {0};
    Clock clock = { .barBeats = &mt };

    setBpm(&clock, 120.0f);

    double unscaled_ticks_per_step_f = get_unscaled_ticks_per_step_f(120.0f);
    u64 expected_ticks_per_step = (u64)(unscaled_ticks_per_step_f * (1 << CLOCK_RESOLUTION_SHIFT));

    TEST_ASSERT_EQUAL_UINT64(expected_ticks_per_step, clock.ticks_per_step);
}

void test_updateClock_should_not_tick_if_not_enough_time_has_passed(void) {
    MusicalTime mt = {0};
    Clock clock = { .barBeats = &mt };
    setBpm(&clock, 120.0f);
    startClock(&clock);

    double unscaled_ticks_per_step_f = get_unscaled_ticks_per_step_f(120.0f);

    // Advance time by less than one step
    mock_advance_system_tick((u64)unscaled_ticks_per_step_f - 1);

    bool ticked = updateClock(&clock);

    TEST_ASSERT_FALSE(ticked);
}

void test_updateClock_should_tick_and_update_musical_time(void) {
    MusicalTime mt = {0};
    Clock clock = { .barBeats = &mt };
    setBpm(&clock, 120.0f);
    startClock(&clock);

    double unscaled_ticks_per_step_f = get_unscaled_ticks_per_step_f(120.0f);

    // Advance time by exactly one step, rounding up to ensure the threshold is met.
    mock_advance_system_tick((u64)ceil(unscaled_ticks_per_step_f));

    bool ticked = updateClock(&clock);

    TEST_ASSERT_TRUE(ticked);
    TEST_ASSERT_EQUAL(1, clock.barBeats->steps);
}

void test_updateClock_accumulator_handles_remainder(void) {
    MusicalTime mt = {0};
    Clock clock = { .barBeats = &mt };
    setBpm(&clock, 120.0f);
    startClock(&clock);

    double unscaled_ticks_per_step_f = get_unscaled_ticks_per_step_f(120.0f);

    // --- First Tick ---
    // Advance by ~1.5 steps. This should cause exactly one tick.
    u64 advance1 = (u64)ceil(unscaled_ticks_per_step_f * 1.5);
    mock_advance_system_tick(advance1);

    bool ticked = updateClock(&clock);
    TEST_ASSERT_TRUE(ticked);
    TEST_ASSERT_EQUAL(1, clock.barBeats->steps);

    // --- Second Tick ---
    // Calculate the total time needed for two full steps from the beginning.
    u64 total_ticks_for_2_steps = (u64)ceil(unscaled_ticks_per_step_f * 2.0);
    // Calculate the remaining time we need to advance to reach the 2-step mark.
    u64 advance2 = total_ticks_for_2_steps - advance1;
    mock_advance_system_tick(advance2);

    ticked = updateClock(&clock);
    TEST_ASSERT_TRUE(ticked); // Should tick again
    TEST_ASSERT_EQUAL(2, clock.barBeats->steps);
}

void test_updateClock_does_not_tick_when_stopped(void) {
    MusicalTime mt = {0};
    Clock clock = { .barBeats = &mt };
    setBpm(&clock, 120.0f);
    startClock(&clock);
    stopClock(&clock);

    double unscaled_ticks_per_step_f = get_unscaled_ticks_per_step_f(120.0f);

    mock_advance_system_tick((u64)unscaled_ticks_per_step_f * 2);

    bool ticked = updateClock(&clock);

    TEST_ASSERT_FALSE(ticked);
}
