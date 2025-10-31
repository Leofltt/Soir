#ifndef VIEWS_H
#define VIEWS_H

#include "clock.h"
#include "track.h"
#include "session_controller.h"

extern void initViews();
extern void deinitViews();
extern void drawStepsBar(int cur_step);
extern void drawTrackbar(Clock *clock, Track *tracks);
extern void drawTracksSequencers(Track *tracks, int cur_step);
extern void drawMainView(Track *tracks, Clock *clock, int selected_row, int selected_col,
                         ScreenFocus focus);
extern void drawClockSettingsView(Clock *clock, int selected_option);
extern void drawQuitMenu(const char *options[], int num_options, int selected_option);
extern void drawTouchScreenSettingsView(int selected_option, ScreenFocus focus);
extern void drawTouchClockSettingsView(Clock *clock, int selected_option);
extern void drawSampleManagerView(int selected_row, int selected_col);
extern void drawStepSettingsView(Session *session, Track *tracks, int selected_row,
                                 int selected_col, int selected_step_option);

#endif // VIEWS_H