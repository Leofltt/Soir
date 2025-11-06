#include "controllers/controller_touch_main.h"
#include "clock.h"
#include "track.h"
#include "engine_constants.h"
#include "session.h"

void handleInputTouchSettings(SessionContext *ctx, u32 kDown, u32 kHeld, u64 now) {
    if (kDown & KEY_LEFT) {
        *ctx->selected_touch_option =
            (*ctx->selected_touch_option > 0) ? *ctx->selected_touch_option - 1 : 1;
    }
    if (kDown & KEY_RIGHT) {
        *ctx->selected_touch_option =
            (*ctx->selected_touch_option < 1) ? *ctx->selected_touch_option + 1 : 0;
    }
    if (kDown & KEY_A && *ctx->selected_touch_option == 0) {
        ctx->session->touch_screen_view   = VIEW_TOUCH_CLOCK_SETTINGS;
        *ctx->selected_touch_clock_option = 0;
    } else if (kDown & KEY_A && *ctx->selected_touch_option == 1) {
        ctx->session->touch_screen_view = VIEW_SAMPLE_MANAGER;
        *ctx->selected_sample_row       = 0;
        *ctx->selected_sample_col       = 0;
    }
    if (kDown & KEY_Y && *ctx->selected_touch_option == 0) {
        LightLock_Lock(ctx->clock_lock);
        if (ctx->clock->status == PLAYING) {
            pauseClock(ctx->clock);
        } else if (ctx->clock->status == PAUSED) {
            resumeClock(ctx->clock);
        }
        LightLock_Unlock(ctx->clock_lock);
    }
    if (kDown & KEY_X && *ctx->selected_touch_option == 0) {
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