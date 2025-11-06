#ifndef VIEWS_MAIN_H
#define VIEWS_MAIN_H

#include "views.h"
#include "clock.h"
#include "track.h"
#include "session_controller.h"

// Function prototypes for views_main.c
extern void drawStepsBar(int cur_step);
extern void drawTrackbar(Clock *clock, Track *tracks);
extern void drawTracksSequencers(Track *tracks, int cur_step);
extern void drawMainView(Track *tracks, Clock *clock, int selected_row, int selected_col,
                         ScreenFocus focus);

#endif // VIEWS_MAIN_H