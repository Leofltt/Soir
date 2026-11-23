#include "controllers/controller_sample_manager.h"
#include "sample_bank.h"
#include "sample_browser.h"
#include "session.h"
#include "sample.h"
#include "ui_constants.h"

static void loadSelectedSample(SessionContext *ctx) {
    int sample_slot = *ctx->selected_sample_row * SAMPLE_GRID_COLS + *ctx->selected_sample_col;
    const char *path_ptr =
        SampleBrowserGetSamplePath(ctx->sample_browser, *ctx->selected_sample_browser_index);
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

void handleInputSampleManager(SessionContext *ctx, u32 kDown, touchPosition touch) {
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
            loadSelectedSample(ctx);
        }
        if (kDown & KEY_B) {
            *ctx->is_selecting_sample = false;
        }
        if (kDown & KEY_TOUCH) {
            float menu_x = (BOTTOM_SCREEN_WIDTH - SAMPLE_BROWSER_WIDTH) / 2.0f;
            float menu_y = (SCREEN_HEIGHT - SAMPLE_BROWSER_HEIGHT) / 2.0f;

            if (touch.px < menu_x || touch.px > menu_x + SAMPLE_BROWSER_WIDTH ||
                touch.py < menu_y || touch.py > menu_y + SAMPLE_BROWSER_HEIGHT) {
                *ctx->is_selecting_sample = false;
            } else {
                float list_y = menu_y + 10.0f;
                if (touch.py >= list_y &&
                    touch.py < list_y + SAMPLE_BROWSER_VISIBLE_ITEMS * BROWSER_ITEM_SPACING) {
                    int visible_row   = (int) ((touch.py - list_y) / BROWSER_ITEM_SPACING);
                    int start_index   = (*ctx->selected_sample_browser_index >= 5)
                                            ? (*ctx->selected_sample_browser_index - 5)
                                            : 0;
                    int clicked_index = start_index + visible_row;
                    int sample_count  = SampleBrowserGetSampleCount(ctx->sample_browser);
                    if (clicked_index >= 0 && clicked_index < sample_count) {
                        if (clicked_index == *ctx->selected_sample_browser_index) {
                            loadSelectedSample(ctx);
                        } else {
                            *ctx->selected_sample_browser_index = clicked_index;
                        }
                    }
                }
            }
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
        if (kDown & KEY_TOUCH) {
            int cell_width  = BOTTOM_SCREEN_WIDTH / SAMPLE_GRID_COLS;
            int cell_height = SCREEN_HEIGHT / SAMPLE_GRID_ROWS;
            int col         = touch.px / cell_width;
            int row         = touch.py / cell_height;

            if (col >= SAMPLE_GRID_COLS)
                col = SAMPLE_GRID_COLS - 1;
            if (row >= SAMPLE_GRID_ROWS)
                row = SAMPLE_GRID_ROWS - 1;

            if (row * SAMPLE_GRID_COLS + col < MAX_SAMPLES) {
                *ctx->selected_sample_row           = row;
                *ctx->selected_sample_col           = col;
                *ctx->is_selecting_sample           = true;
                *ctx->selected_sample_browser_index = 0;
            }
        }
    }
}