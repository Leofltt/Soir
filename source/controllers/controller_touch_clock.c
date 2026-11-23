#include "controllers/controller_touch_clock.h"
#include "audio_utils.h"
#include "clock.h"
#include "session.h"
#include "controllers/session_controller.h"
#include "event_queue.h"
#include "ui_constants.h"

static void adjustClockOption(SessionContext *ctx, int delta) {
    if (*ctx->selected_touch_clock_option == 0) { // BPM
        float new_bpm = ctx->clock->bpm + delta;
        if (new_bpm < 20)
            new_bpm = 20;
        if (new_bpm > 200)
            new_bpm = 200;
        Event event             = { .type = SET_BPM };
        event.data.bpm_data.bpm = new_bpm;
        eventQueuePush(ctx->event_queue, event);
    } else if (*ctx->selected_touch_clock_option == 1) { // Beats per bar
        int new_beats = ctx->clock->barBeats->beats_per_bar + delta;
        if (new_beats < 2)
            new_beats = 2;
        if (new_beats > 16)
            new_beats = 16;
        Event event                 = { .type = SET_BEATS_PER_BAR };
        event.data.beats_data.beats = new_beats;
        eventQueuePush(ctx->event_queue, event);
    }
}

void handleInputTouchClock(SessionContext *ctx, u32 kDown, u32 kHeld, touchPosition touch,
                           u64 now) {
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
        adjustClockOption(ctx, -1);
    }
    if (handle_continuous_press(kDown, kHeld, now, KEY_RIGHT, ctx->right_timer,
                                ctx->HOLD_DELAY_INITIAL, ctx->HOLD_DELAY_REPEAT)) {
        adjustClockOption(ctx, 1);
    }

    float menu_x = (BOTTOM_SCREEN_WIDTH - CLOCK_MENU_WIDTH) / 2.0f;
    float menu_y = (SCREEN_HEIGHT - CLOCK_MENU_HEIGHT) / 2.0f;

    if (kDown & KEY_TOUCH) {
        if (touch.px < menu_x || touch.px > menu_x + CLOCK_MENU_WIDTH || touch.py < menu_y ||
            touch.py > menu_y + CLOCK_MENU_HEIGHT) {
            ctx->session->touch_screen_view = VIEW_TOUCH_SETTINGS;
            return;
        }
        if (touch.py >= menu_y + 65 && touch.py <= menu_y + 95) { // Row 2: Back
            *ctx->selected_touch_clock_option = 2;
            ctx->session->touch_screen_view   = VIEW_TOUCH_SETTINGS;
            return;
        }
    }

    if (touch.px >= menu_x && touch.px <= menu_x + CLOCK_MENU_WIDTH && touch.py >= menu_y + 15 &&
        touch.py < menu_y + 65) {
        if (handle_continuous_press(kDown, kHeld, now, KEY_TOUCH, ctx->up_timer,
                                    ctx->HOLD_DELAY_INITIAL, ctx->HOLD_DELAY_REPEAT)) {
            *ctx->selected_touch_clock_option = (touch.py < menu_y + 40) ? 0 : 1;
            adjustClockOption(ctx, (touch.px < BOTTOM_SCREEN_WIDTH / 2) ? -1 : 1);
        }
    }
}