#ifndef UI_H
#define UI_H

#include "clock.h"
#include "track.h"

typedef enum ScreenFocus ScreenFocus;
typedef struct Session   Session;
#include "sample_bank.h"
#include "sample_browser.h"
#include <citro2d.h>

#define MAX_VIEW_PARAMS 16

typedef enum {
    PARAM_TYPE_GENERIC,
    PARAM_TYPE_FLOAT_0_1,   // e.g., Volume
    PARAM_TYPE_FLOAT_N1_1,  // e.g., Pan
    PARAM_TYPE_HZ,          // e.g., Filter Cutoff
    PARAM_TYPE_MIDI_NOTE,   // e.g., Osc Freq
    PARAM_TYPE_WAVEFORM,    // e.g., SubSynth waveform
    PARAM_TYPE_FILTER_TYPE, // e.g., Filter Type
    PARAM_TYPE_SAMPLE_INDEX,
    PARAM_TYPE_PLAYBACK_MODE,
    PARAM_TYPE_MOD_RATIO,       // For FM_SYNTH
    PARAM_TYPE_ENVELOPE_BUTTON, // This replaces your bool is_envelope_button
    PARAM_TYPE_INT
    // ... add more types as needed
} ParameterType;

typedef struct {
    const char   *label;            // "Volume", "MIDI Note", etc.
    char          value_string[64]; // The pre-formatted value to draw
    int           unique_id;        // 0-indexed ID for selection
    int           column;           // 0 or 1
    int           row_in_column;    // 0, 1, 2...
    ParameterType type;             // Our new enum
} ParameterInfo;

extern C2D_Font    font_angular;
extern C2D_Font    font_heavy;
extern C2D_TextBuf text_buf;
extern C2D_Text    text_obj;

extern void initViews();
extern void deinitViews();
extern void drawStepsBar(int cur_step, int steps_per_beat);
extern void drawTrackbar(Clock *clock, Track *tracks);
extern void drawTracksSequencers(Track *tracks, int cur_step);
extern void drawMainView(Track *tracks, Clock *clock, int selected_row, int selected_col,
                         ScreenFocus focus);
extern void drawClockSettingsView(Clock *clock, int selected_option);
extern void drawQuitMenu(const char *options[], int num_options, int selected_option);
extern void drawTouchScreenSettingsView(int selected_option, ScreenFocus focus);
extern void drawTouchClockSettingsView(Clock *clock, int selected_option);
extern void drawSampleManagerView(SampleBank *bank, int selected_row, int selected_col,
                                  bool is_selecting_sample, int selected_sample_browser_index,
                                  SampleBrowser *browser, ScreenFocus focus);
extern void drawStepSettingsView(Session *session, Track *tracks, int selected_row,
                                 int selected_col, int selected_step_option,
                                 SampleBank *sample_bank, ScreenFocus focus);
extern void drawStepSettingsEditView(Track *track, TrackParameters *params,
                                     int selected_step_option, int selected_adsr_option,
                                     SampleBank *sample_bank);

extern int generateParameterList(Track *track, TrackParameters *params, SampleBank *sample_bank,
                                 ParameterInfo *list_buffer, int max_params);

#endif // UI_H