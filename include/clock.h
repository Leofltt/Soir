#ifndef CLOCK_H
#define CLOCK_H

#include <3ds/types.h>

// note resolution (I'd recc 2 * 3 * maxSubDivisionNeeded, ex. 4 for 16th notes)
#define STEPS_PER_BEAT (2 * 3 * 4)

typedef enum { STOPPED = 0, PLAYING = 1, PAUSED = 2 } ClockStatus;

extern const char* clockStatusName[];

typedef struct {
    int bar;
    int beat;
    int deltaStep;
    int steps;
    int beats_per_bar;
} MusicalTime;

typedef struct {
    float bpm;
    float ticks_per_beat;
    float ticks;
    int ticks_per_step;
    ClockStatus status;
    MusicalTime* barBeats;
} Clock;

extern void set_bpm(Clock* clock, float bpm);
extern bool update_clock(Clock* clock);
extern void reset_clock(Clock* clock);
extern void stop_clock(Clock* clock);
extern void pause_clock(Clock* clock);
extern void start_clock(Clock* clock);
extern void reset_barBeats(Clock* clock);

#endif  // CLOCK_H