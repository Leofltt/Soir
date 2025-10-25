#ifndef VIEWS_H
#define VIEWS_H

#include "clock.h"
#include "track.h"

extern void initViews();
extern void deinitViews();
extern void drawStepsBar(int cur_step);
extern void drawTrackbar(Clock *clock);
extern void drawMainView(Track *tracks, Clock *clock);

#endif // VIEWS_H