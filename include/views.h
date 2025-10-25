#ifndef VIEWS_H
#define VIEWS_H

#include "clock.h"
#include "track.h"

extern void initViews();
extern void deinitViews();
extern void drawStepsBar(int cur_step);
extern void drawTrackbar(Clock *clock, Track *tracks);
extern void drawTracksSequencers(Track *tracks, int cur_step);
extern void drawMainView(Track *tracks, Clock *clock, int selected_row, int selected_col);

#endif // VIEWS_H