#include "session_controller.h"
#include "ui/ui.h"
#include "audio_utils.h"
#include <stdbool.h>
#include <string.h>
#include "engine_constants.h"
#include "controllers/controller_main.h"
#include "controllers/controller_settings.h"
#include "controllers/controller_quit.h"
#include "controllers/controller_step_edit.h"
#include "controllers/controller_touch_main.h"
#include "controllers/controller_touch_clock.h"
#include "controllers/controller_sample_manager.h"
#include "controllers/controller_step_settings.h"

bool handle_continuous_press(u32 kDown, u32 kHeld, u64 now, u32 key, u64 *timer,
                             const u64 delay_initial, const u64 delay_repeat) {
    if (!timer) {
        return false;
    }
    if (kDown & key) {
        *timer = now + delay_initial;
        return true;
    }
    if ((kHeld & key) && (now >= *timer)) {
        *timer = now + delay_repeat;
        return true;
    }
    return false;
}

void sessionControllerHandleInput(SessionContext *ctx, u32 kDown, u32 kHeld, u64 now,
                                  bool *should_break_loop) {
    if (kDown & KEY_START) {
        *ctx->previous_screen_focus    = *ctx->screen_focus;
        *ctx->screen_focus             = FOCUS_TOP;
        ctx->session->main_screen_view = VIEW_QUIT;
        *ctx->selected_quit_option     = 0;
        LightLock_Lock(ctx->clock_lock);
        pauseClock(ctx->clock);
        LightLock_Unlock(ctx->clock_lock);
    }

    if (ctx->session->main_screen_view != VIEW_QUIT) {
        if (kDown & KEY_R) {
            *ctx->screen_focus = (*ctx->screen_focus == FOCUS_TOP) ? FOCUS_BOTTOM : FOCUS_TOP;
        }
    }

    if (kDown & KEY_L) {
        if (ctx->session->touch_screen_view == VIEW_TOUCH_SETTINGS) {
            ctx->session->previous_touch_screen_view = VIEW_TOUCH_SETTINGS;
            ctx->session->touch_screen_view          = VIEW_STEP_SETTINGS;
        } else if (ctx->session->touch_screen_view == VIEW_STEP_SETTINGS) {
            if (ctx->session->previous_touch_screen_view == VIEW_SAMPLE_MANAGER) {
                ctx->session->touch_screen_view = VIEW_SAMPLE_MANAGER;
            } else {
                ctx->session->previous_touch_screen_view = VIEW_TOUCH_SETTINGS;
                ctx->session->touch_screen_view          = VIEW_TOUCH_SETTINGS;
            }
        } else if (ctx->session->touch_screen_view == VIEW_SAMPLE_MANAGER) {
            ctx->session->previous_touch_screen_view = VIEW_SAMPLE_MANAGER;
            ctx->session->touch_screen_view          = VIEW_STEP_SETTINGS;
        }
    }

    if (*ctx->screen_focus == FOCUS_TOP) {
        switch (ctx->session->main_screen_view) {
        case VIEW_MAIN:
            handleInputMainView(ctx, kDown, kHeld, now);
            break;
        case VIEW_SETTINGS:
            handleInputSettingsView(ctx, kDown, kHeld, now);
            break;
        case VIEW_QUIT:
            handleInputQuitView(ctx, kDown, should_break_loop);
            break;
        case VIEW_STEP_SETTINGS_EDIT:
            handleInputStepEditView(ctx, kDown, kHeld, now);
            break;
        case VIEW_ABOUT:
            // No handler yet
            break;
        }
    } else if (*ctx->screen_focus == FOCUS_BOTTOM) {
        switch (ctx->session->touch_screen_view) {
        case VIEW_TOUCH_SETTINGS:
            handleInputTouchSettings(ctx, kDown, kHeld, now);
            break;
        case VIEW_TOUCH_CLOCK_SETTINGS:
            handleInputTouchClock(ctx, kDown, kHeld, now);
            break;
        case VIEW_SAMPLE_MANAGER:
            handleInputSampleManager(ctx, kDown);
            break;
        case VIEW_STEP_SETTINGS:
            handleInputStepSettings(ctx, kDown);
            break;
        default:
            break;
        }
    }
}