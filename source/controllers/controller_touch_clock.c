#include "controllers/controller_touch_clock.h"
#include "audio_utils.h"
#include "clock.h"
#include "session.h"
#include "controllers/session_controller.h"
#include "event_queue.h"

void handleInputTouchClock(SessionContext *ctx, u32 kDown, u32 kHeld, u64 now) {
    if (kDown & KEY_UP) {
        *ctx->selected_touch_clock_option =
            (*ctx->selected_touch_clock_option > 0) ? *ctx->selected_touch_clock_option - 1 : 2;
    }
    if (kDown & KEY_DOWN) {
        *ctx->selected_touch_clock_option =
            (*ctx->selected_touch_clock_option < 2) ? *ctx->selected_touch_clock_option + 1 : 0;
    }
    if (kDown & KEY_B) {
        ctx->session->touch_screen_view = VIEW_TOUCH_SETTINGS;
    }
    if (kDown & KEY_A && *ctx->selected_touch_clock_option == 2) {
        ctx->session->touch_screen_view = VIEW_TOUCH_SETTINGS;
    }
    if (handle_continuous_press(kDown, kHeld, now, KEY_LEFT, ctx->left_timer,
                                ctx->HOLD_DELAY_INITIAL, ctx->HOLD_DELAY_REPEAT)) {
        if (*ctx->selected_touch_clock_option == 0) { // BPM
            float new_bpm = ctx->clock->bpm - 1;
            if (new_bpm < 20)
                new_bpm = 20;
            Event event             = { .type = SET_BPM };
            event.data.bpm_data.bpm = new_bpm;
            eventQueuePush(ctx->event_queue, event);
        } else if (*ctx->selected_touch_clock_option == 1) { // Beats per bar
            int new_beats = ctx->clock->barBeats->beats_per_bar - 1;
            if (new_beats < 2)
                new_beats = 2;
            Event event                 = { .type = SET_BEATS_PER_BAR };
            event.data.beats_data.beats = new_beats;
            eventQueuePush(ctx->event_queue, event);
        }
    }
    if (handle_continuous_press(kDown, kHeld, now, KEY_RIGHT, ctx->right_timer,
                                ctx->HOLD_DELAY_INITIAL, ctx->HOLD_DELAY_REPEAT)) {
        if (*ctx->selected_touch_clock_option == 0) { // BPM
            float new_bpm = ctx->clock->bpm + 1;
            if (new_bpm > 200)
                new_bpm = 200;
            Event event             = { .type = SET_BPM };
            event.data.bpm_data.bpm = new_bpm;
            eventQueuePush(ctx->event_queue, event);
        } else if (*ctx->selected_touch_clock_option == 1) { // Beats per bar
            int new_beats = ctx->clock->barBeats->beats_per_bar + 1;
            if (new_beats > 16)
                new_beats = 16;
            Event event                 = { .type = SET_BEATS_PER_BAR };
            event.data.beats_data.beats = new_beats;
            eventQueuePush(ctx->event_queue, event);
        }
    }
}