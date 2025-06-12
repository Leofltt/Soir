#ifndef CLOCK_H
#define CLOCK_H

#ifdef TESTING
#include "../tests/mock_3ds.h"
#else
#include <3ds/types.h>
#endif

#define MAXSUBDIVBEAT 8
// note resolution (I'd recc 2 * 3 * maxSubDivisionNeeded, ex. 4 for 16th notes)
#define STEPS_PER_BEAT (2 * 3 * MAXSUBDIVBEAT)

typedef enum { STOPPED = 0, PLAYING = 1, PAUSED = 2 } ClockStatus;

extern const char *clockStatusName[];

typedef struct {
    int bar;
    int beat;
    int deltaStep;
    int steps;
    int beats_per_bar;
} MusicalTime;

typedef struct {
    float        bpm;
    float        ticks_per_beat;
    float        ticks;
    int          ticks_per_step;
    ClockStatus  status;
    MusicalTime *barBeats;
} Clock;

extern void setBpm(Clock *clock, float bpm);
extern bool updateClock(Clock *clock);
extern void resetClock(Clock *clock);
extern void stopClock(Clock *clock);
extern void pauseClock(Clock *clock);
extern void startClock(Clock *clock);
extern void resetBarBeats(Clock *clock);

#endif // CLOCK_H