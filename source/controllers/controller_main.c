#include "controllers/controller_main.h"
#include "audio_utils.h"
#include "engine_constants.h"
#include "clock.h"
#include "track.h"
#include "session.h"
#include "controllers/session_controller.h"

void handleInputMainView(SessionContext *ctx, u32 kDown, u32 kHeld, u64 now) {
    if (handle_continuous_press(kDown, kHeld, now, KEY_UP, ctx->up_timer, ctx->HOLD_DELAY_INITIAL,
                                ctx->HOLD_DELAY_REPEAT)) {
        *ctx->selected_row = (*ctx->selected_row > 0) ? *ctx->selected_row - 1 : N_TRACKS;
    }

    if (handle_continuous_press(kDown, kHeld, now, KEY_DOWN, ctx->down_timer,
                                ctx->HOLD_DELAY_INITIAL, ctx->HOLD_DELAY_REPEAT)) {
        *ctx->selected_row = (*ctx->selected_row < N_TRACKS) ? *ctx->selected_row + 1 : 0;
    }

    if (handle_continuous_press(kDown, kHeld, now, KEY_LEFT, ctx->left_timer,
                                ctx->HOLD_DELAY_INITIAL, ctx->HOLD_DELAY_REPEAT)) {
        *ctx->selected_col = (*ctx->selected_col > 0) ? *ctx->selected_col - 1 : 16;
    }

    if (handle_continuous_press(kDown, kHeld, now, KEY_RIGHT, ctx->right_timer,
                                ctx->HOLD_DELAY_INITIAL, ctx->HOLD_DELAY_REPEAT)) {
        *ctx->selected_col = (*ctx->selected_col < 16) ? *ctx->selected_col + 1 : 0;
    }

    if (kDown & KEY_Y) {
        if (*ctx->selected_row == 0 && *ctx->selected_col == 0) {
            LightLock_Lock(ctx->clock_lock);
            if (ctx->clock->status == PLAYING) {
                pauseClock(ctx->clock);
            } else if (ctx->clock->status == PAUSED) {
                resumeClock(ctx->clock);
            }
            LightLock_Unlock(ctx->clock_lock);
        } else if (*ctx->selected_row > 0 && *ctx->selected_col > 0) {
            ctx->session->touch_screen_view = VIEW_STEP_SETTINGS;
            *ctx->screen_focus              = FOCUS_BOTTOM;
        }
    }

    if (kDown & KEY_A) {
        if (*ctx->selected_row == 0 && *ctx->selected_col == 0) {
            LightLock_Lock(ctx->clock_lock);
            stopClock(ctx->clock);
            LightLock_Unlock(ctx->clock_lock);
            ctx->session->main_screen_view = VIEW_SETTINGS;
            *ctx->selected_settings_option = 0;
        } else if (*ctx->selected_row > 0 && *ctx->selected_col == 0) {
            int track_index = *ctx->selected_row - 1;
            if (track_index < N_TRACKS) {
                LightLock_Lock(ctx->tracks_lock);
                ctx->tracks[track_index].is_muted = !ctx->tracks[track_index].is_muted;
                for (int i = 0; i < ctx->tracks[track_index].sequencer->n_beats *
                                        ctx->tracks[track_index].sequencer->steps_per_beat;
                     i++) {
                    ctx->tracks[track_index].sequencer->steps[i].data->is_muted =
                        ctx->tracks[track_index].is_muted;
                }
                LightLock_Unlock(ctx->tracks_lock);
            }
        } else if (*ctx->selected_col > 0) {
            int step_index = *ctx->selected_col - 1;
            LightLock_Lock(ctx->tracks_lock);
            if (*ctx->selected_row == 0) { // Header row
                for (int i = 0; i < N_TRACKS; i++) {
                    if (ctx->tracks[i].sequencer && ctx->tracks[i].sequencer->steps) {
                        ctx->tracks[i].sequencer->steps[step_index].active =
                            !ctx->tracks[i].sequencer->steps[step_index].active;
                    }
                }
            } else { // Track row
                int track_index = *ctx->selected_row - 1;
                if (track_index < N_TRACKS && ctx->tracks[track_index].sequencer &&
                    ctx->tracks[track_index].sequencer->steps) {
                    ctx->tracks[track_index].sequencer->steps[step_index].active =
                        !ctx->tracks[track_index].sequencer->steps[step_index].active;
                }
            }
            LightLock_Unlock(ctx->tracks_lock);
        }
    }

    if (kDown & KEY_X) {
        if (*ctx->selected_row == 0 && *ctx->selected_col == 0) {
            LightLock_Lock(ctx->clock_lock);
            if (ctx->clock->status == PLAYING || ctx->clock->status == PAUSED) {
                stopClock(ctx->clock);
                LightLock_Lock(ctx->tracks_lock);
                for (int i = 0; i < N_TRACKS; i++) {
                    if (ctx->tracks[i].sequencer) {
                        ctx->tracks[i].sequencer->cur_step = 0;
                    }
                }
                LightLock_Unlock(ctx->tracks_lock);
            } else {
                startClock(ctx->clock);
            }
            LightLock_Unlock(ctx->clock_lock);
        }
    }
}