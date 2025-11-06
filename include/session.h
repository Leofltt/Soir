#ifndef SESSION_H
#define SESSION_H

#include <3ds.h>
#include <stdbool.h>
#include "track.h"
#include "clock.h"
#include "sample_bank.h"
#include "sample_browser.h"
#include "event_queue.h"
#include "track_parameters.h"
#include "synth.h"
#include "samplers.h"
#include "ui/ui.h"

typedef enum {
    VIEW_MAIN               = 0,
    VIEW_SETTINGS           = 1,
    VIEW_ABOUT              = 2,
    VIEW_QUIT               = 3,
    VIEW_STEP_SETTINGS_EDIT = 4
} TopScreenView;

typedef enum {
    VIEW_TOUCH_SETTINGS       = 0,
    VIEW_TOUCH_CLOCK_SETTINGS = 1,
    VIEW_SAMPLE_MANAGER       = 2,
    VIEW_STEP_SETTINGS        = 3
} BottomScreenView;

enum ScreenFocus { FOCUS_TOP, FOCUS_BOTTOM };
typedef enum ScreenFocus ScreenFocus;

struct Session {
    TopScreenView    main_screen_view;
    BottomScreenView touch_screen_view;
    BottomScreenView previous_touch_screen_view;
};
typedef struct Session Session;

typedef struct {
    // Pointers to main() local state
    Session *session;
    int     *selected_row;
    int     *selected_col;
    int     *selected_settings_option;
    int     *selected_touch_option;
    int     *selected_touch_clock_option;
    int     *selected_sample_row;
    int     *selected_sample_col;
    int     *selected_sample_browser_index;
    int     *selected_step_option;
    int     *selected_adsr_option;
    int     *selected_quit_option;
    bool    *is_selecting_sample;

    u64 *up_timer;
    u64 *down_timer;
    u64 *left_timer;
    u64 *right_timer;

    ScreenFocus     *screen_focus;
    ScreenFocus     *previous_screen_focus;
    BottomScreenView previous_touch_screen_view;

    // Pointers to global state
    Track                 *tracks;
    Clock                 *clock;
    EventQueue            *event_queue;
    SampleBank            *sample_bank;
    SampleBrowser         *sample_browser;
    TrackParameters       *editing_step_params;
    SubSynthParameters    *editing_subsynth_params;
    OpusSamplerParameters *editing_sampler_params;
    FMSynthParameters     *editing_fm_synth_params;
    LightLock             *clock_lock;
    LightLock             *tracks_lock;

    int           last_edited_param_unique_id;
    ParameterType last_edited_param_type;
    const char   *last_edited_param_label;

    // Hold constants
    const u64 HOLD_DELAY_INITIAL;
    const u64 HOLD_DELAY_REPEAT;

} SessionContext;

#endif // SESSION_H