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
        Event event = { .type = (ctx->clock->status == PLAYING) ? PAUSE_CLOCK : RESUME_CLOCK };
        eventQueuePush(ctx->event_queue, event);
    }
    if (kDown & KEY_X && *ctx->selected_touch_option == 0) {
        Event event = { .type = (ctx->clock->status == PLAYING || ctx->clock->status == PAUSED)
                                    ? STOP_CLOCK
                                    : START_CLOCK };
        eventQueuePush(ctx->event_queue, event);

        if (event.type == STOP_CLOCK) {
            Event resetEvent = { .type = RESET_SEQUENCERS };
            eventQueuePush(ctx->event_queue, resetEvent);
        }
    }
}