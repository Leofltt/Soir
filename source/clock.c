#include "clock.h"

#include <3ds/os.h>
#include <math.h>

const char *clockStatusName[] = { "Stopped", "Playing", "Paused" };

void resetBarBeats(Clock *clock) {
    if (clock->barBeats) {
        clock->barBeats->bar       = 0;
        clock->barBeats->beat      = 0;
        clock->barBeats->deltaStep = 0;
        clock->barBeats->steps     = 0;
    }
}

void resetClock(Clock *clock) {
    clock->manual_tick_counter = 0;
};
void stopClock(Clock *clock) {
    clock->status = STOPPED;
    resetBarBeats(clock);
};
void pauseClock(Clock *clock) {
    clock->status = PAUSED;
};
void startClock(Clock *clock) {
    clock->status = PLAYING;
    resetBarBeats(clock);
    resetClock(clock);
};

// This function is temporarily disabled for diagnostic purposes.
void setBpm(Clock *clock, float bpm) {
    // Empty.
}

// This is a temporary, manual clock for diagnostic purposes.
// It ignores system time and triggers a step after a fixed number of calls.
bool updateClock(Clock *clock) {
    if (!clock || clock->status != PLAYING) {
        return false;
    }

    clock->manual_tick_counter++;

    // The clock thread polls every 1ms. Let's trigger a step every ~21ms (for ~120 BPM).
    if (clock->manual_tick_counter >= 21) {
        clock->manual_tick_counter = 0;

        // update musical time
        clock->barBeats->steps += 1;
        int totBeats               = clock->barBeats->steps / STEPS_PER_BEAT;
        clock->barBeats->bar       = totBeats / clock->barBeats->beats_per_bar;
        clock->barBeats->beat      = (totBeats % clock->barBeats->beats_per_bar);
        clock->barBeats->deltaStep = clock->barBeats->steps % STEPS_PER_BEAT;

        return true;
    }

    return false;
}