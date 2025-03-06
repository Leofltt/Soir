#include "clock.h"

void reset_clock(Clock* clock) {
    clock->ticks = 0;
};
void stop_clock(Clock* clock) {
    clock->status = STOPPED;
    reset_clock(clock);
};
void pause_clock(Clock* clock) {
    clock->status = PAUSED;
};
void start_clock(Clock* clock) {
    clock->status = PLAYING;
};

void set_bpm(Clock* clock, float bpm) {
    clock->bpm = bpm;
    clock->ticks_per_beat = SYSCLOCK_ARM11 * 60.0 / clock->bpm;
    clock->ticks_per_step = clock->ticks_per_beat / clock->steps_per_beat;
}

void update_clock(Clock* clock) {
    if (clock->status == PLAYING) {
        clock->ticks++;
    }
}