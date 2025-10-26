#include "clock.h"

#ifndef TESTING
#include <3ds/os.h>
#endif
#include <math.h>

// We use scaled integers to avoid floating point errors. 8 bits of fractional precision.
#define CLOCK_RESOLUTION_SHIFT 8

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
    clock->last_tick_time = svcGetSystemTick();
    clock->time_accumulator = 0;
};
void stopClock(Clock *clock) {
    clock->status = STOPPED;
    resetBarBeats(clock);
};
void pauseClock(Clock *clock) {
    clock->status = PAUSED;
};

void resumeClock(Clock *clock) {
    if (clock->status == PAUSED) {
        clock->status         = PLAYING;
        clock->last_tick_time = svcGetSystemTick();
    }
}

void startClock(Clock *clock) {
    clock->status = PLAYING;
    resetBarBeats(clock);
    resetClock(clock);
    if (clock->ticks_per_step > 0) { // Avoid issues if bpm is 0
        clock->time_accumulator = clock->ticks_per_step;
    }
};

void setBpm(Clock *clock, float bpm) {
    if (clock && clock->bpm != bpm && bpm >= 20 && bpm <= 200) {
        clock->bpm = bpm;
        // Use double for intermediate calculation for precision
        double ticks_per_beat = (double)SYSCLOCK_ARM11 * 60.0 / bpm;
        double ticks_per_step_f = ticks_per_beat / STEPS_PER_BEAT;

        // Scale up to maintain fractional precision with integers
        clock->ticks_per_step = (u64)(ticks_per_step_f * (1 << CLOCK_RESOLUTION_SHIFT));
        resetClock(clock);
    }
}

void setBeatsPerBar(Clock *clock, int beats) {
    if (clock && clock->barBeats && beats >= 2 && beats <= 16) {
        clock->barBeats->beats_per_bar = beats;
    }
}

bool updateClock(Clock *clock) {
    if (!clock || clock->status != PLAYING) {
        return false;
    }

    u64 now = svcGetSystemTick();
    u64 delta_ticks = now - clock->last_tick_time;
    clock->last_tick_time = now;

    // Scale up delta_ticks before adding to the accumulator
    clock->time_accumulator += (delta_ticks << CLOCK_RESOLUTION_SHIFT);

    if (clock->time_accumulator >= clock->ticks_per_step) {
        clock->time_accumulator -= clock->ticks_per_step;

        // update musical time
        int totBeats               = clock->barBeats->steps / STEPS_PER_BEAT;
        clock->barBeats->bar       = totBeats / clock->barBeats->beats_per_bar;
        clock->barBeats->beat      = (totBeats % clock->barBeats->beats_per_bar);
        clock->barBeats->deltaStep = clock->barBeats->steps % STEPS_PER_BEAT;
        clock->barBeats->steps++;

        return true;
    }

    return false;
}