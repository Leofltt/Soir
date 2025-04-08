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
    uint64_t now = svcGetSystemTick();
    clock->ticks = now;
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
    resetClock(clock);
};

void setBpm(Clock *clock, float bpm) {
    if (clock && clock->bpm != bpm) {
        clock->bpm            = bpm;
        clock->ticks_per_beat = SYSCLOCK_ARM11 * 60.0 / bpm;
        clock->ticks_per_step = clock->ticks_per_beat / STEPS_PER_BEAT;
        resetClock(clock);
    }
}

bool updateClock(Clock *clock) {
    if (!clock || clock->status != PLAYING) {
        return false;
    }

    uint64_t now                       = svcGetSystemTick();
    bool     shouldUpdateStepSequencer = (now - clock->ticks >= clock->ticks_per_step);

    if (shouldUpdateStepSequencer) {
        clock->ticks = now;
        // step = (step + 1) % (STEPS_PER_BEAT * BEATS_PER_BAR);  // Loop through steps

        // update musical time
        clock->barBeats->steps += 1;
        int totBeats               = clock->barBeats->steps / STEPS_PER_BEAT;
        clock->barBeats->bar       = floor(totBeats / clock->barBeats->beats_per_bar);
        clock->barBeats->beat      = (totBeats % clock->barBeats->beats_per_bar);
        clock->barBeats->deltaStep = clock->barBeats->steps % STEPS_PER_BEAT;
    }
    return shouldUpdateStepSequencer;
}