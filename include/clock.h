#ifndef CLOCK_H
#define CLOCK_H

#include <3ds.h>

typedef enum { STOPPED = 0, PLAYING = 1, PAUSED = 2 } ClockStatus;

typedef struct {
    float bpm;
    float steps_per_beat; // note resolution
    float ticks_per_beat;
    float ticks;
    float ticks_per_step;
    ClockStatus status;
} Clock;

extern void set_bpm(Clock* clock, float bpm);
extern void update_clock(Clock* clock);
extern void reset_clock(Clock* clock);
extern void stop_clock(Clock* clock);
extern void pause_clock(Clock* clock);
extern void start_clock(Clock* clock);

#endif // CLOCK_H