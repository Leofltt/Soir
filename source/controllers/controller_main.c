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
                Event event                = { .type = SET_MUTE, .track_id = track_index };
                event.data.mute_data.muted = !ctx->tracks[track_index].is_muted;
                eventQueuePush(ctx->event_queue, event);
            }
        } else if (*ctx->selected_col > 0) {
            int step_index = *ctx->selected_col - 1;
            if (*ctx->selected_row == 0) { // Header row
                for (int i = 0; i < N_TRACKS; i++) {
                    Event event                         = { .type = TOGGLE_STEP, .track_id = i };
                    event.data.toggle_step_data.step_id = step_index;
                    eventQueuePush(ctx->event_queue, event);
                }
            } else { // Track row
                int   track_index = *ctx->selected_row - 1;
                Event event       = { .type = TOGGLE_STEP, .track_id = track_index };
                event.data.toggle_step_data.step_id = step_index;
                eventQueuePush(ctx->event_queue, event);
            }
        }
    }

    if (kDown & KEY_X) {
        if (*ctx->selected_row == 0 && *ctx->selected_col == 0) {
            LightLock_Lock(ctx->clock_lock);
            if (ctx->clock->status == PLAYING || ctx->clock->status == PAUSED) {
                stopClock(ctx->clock);
                Event event = { .type = RESET_SEQUENCERS };
                eventQueuePush(ctx->event_queue, event);
            } else {
                startClock(ctx->clock);
            }
            LightLock_Unlock(ctx->clock_lock);
        }
    }
}