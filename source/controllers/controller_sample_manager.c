#include "controllers/controller_sample_manager.h"
#include "sample_bank.h"
#include "sample_browser.h"
#include "session.h"
#include "sample.h"

void handleInputSampleManager(SessionContext *ctx, u32 kDown) {
    if (*ctx->is_selecting_sample) {
        if (kDown & KEY_UP) {
            *ctx->selected_sample_browser_index =
                (*ctx->selected_sample_browser_index > 0)
                    ? *ctx->selected_sample_browser_index - 1
                    : SampleBrowserGetSampleCount(ctx->sample_browser) - 1;
        }
        if (kDown & KEY_DOWN) {
            *ctx->selected_sample_browser_index =
                (*ctx->selected_sample_browser_index <
                 SampleBrowserGetSampleCount(ctx->sample_browser) - 1)
                    ? *ctx->selected_sample_browser_index + 1
                    : 0;
        }
        if (kDown & KEY_A) {
            int         sample_slot = *ctx->selected_sample_row * 4 + *ctx->selected_sample_col;
            const char *path_ptr    = SampleBrowserGetSamplePath(ctx->sample_browser,
                                                                 *ctx->selected_sample_browser_index);
            if (path_ptr != NULL) {
                Sample *new_sample = sample_create(path_ptr);

                if (new_sample) {
                    Event event                                = { .type = SWAP_SAMPLE };
                    event.data.swap_sample_data.slot_id        = sample_slot;
                    event.data.swap_sample_data.new_sample_ptr = new_sample;

                    eventQueuePush(ctx->event_queue, event);
                    *ctx->is_selecting_sample = false;
                } else {
                    *ctx->is_selecting_sample = false;
                }
            }
        }
        if (kDown & KEY_B) {
            *ctx->is_selecting_sample = false;
        }
    } else {
        if (kDown & KEY_UP) {
            *ctx->selected_sample_row =
                (*ctx->selected_sample_row > 0) ? *ctx->selected_sample_row - 1 : 2;
        }
        if (kDown & KEY_DOWN) {
            *ctx->selected_sample_row =
                (*ctx->selected_sample_row < 2) ? *ctx->selected_sample_row + 1 : 0;
        }
        if (kDown & KEY_LEFT) {
            *ctx->selected_sample_col =
                (*ctx->selected_sample_col > 0) ? *ctx->selected_sample_col - 1 : 3;
        }
        if (kDown & KEY_RIGHT) {
            *ctx->selected_sample_col =
                (*ctx->selected_sample_col < 3) ? *ctx->selected_sample_col + 1 : 0;
        }
        if (kDown & KEY_A) {
            *ctx->is_selecting_sample           = true;
            *ctx->selected_sample_browser_index = 0;
        }
        if (kDown & KEY_B) {
            ctx->session->touch_screen_view = VIEW_TOUCH_SETTINGS;
        }
    }
}