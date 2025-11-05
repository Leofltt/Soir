#ifndef SESSION_CONTROLLER_H
#define SESSION_CONTROLLER_H

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

/**
 * @brief Checks for a continuous key press (down or held).
 * @return true if the key is newly pressed OR if the hold repeat delay has passed.
 */
bool handle_continuous_press(u32 kDown, u32 kHeld, u64 now, u32 key, u64 *timer,
                             const u64 delay_initial, const u64 delay_repeat);

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

typedef enum { FOCUS_TOP, FOCUS_BOTTOM } ScreenFocus;

typedef struct {
    TopScreenView    main_screen_view;
    BottomScreenView touch_screen_view;

} Session;

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
    int     *selectedQuitOption;
    bool    *is_selecting_sample;

    u64 *up_timer;
    u64 *down_timer;
    u64 *left_timer;
    u64 *right_timer;

    ScreenFocus *screen_focus;
    ScreenFocus *previous_screen_focus;

    // Pointers to global state
    Track                 *tracks;
    Clock                 *clock;
    EventQueue            *event_queue;
    SampleBank            *sample_bank;
    SampleBrowser         *sample_browser;
    TrackParameters       *editing_step_params;
    SubSynthParameters    *editing_subsynth_params;
    OpusSamplerParameters *editing_sampler_params;
    LightLock             *clock_lock;
    LightLock             *tracks_lock;

    // Hold constants
    const u64 HOLD_DELAY_INITIAL;
    const u64 HOLD_DELAY_REPEAT;

} SessionContext;

void sessionControllerHandleInput(SessionContext *ctx, u32 kDown, u32 kHeld, u64 now,
                                  bool *should_break_loop);

#endif