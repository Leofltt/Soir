#include "ui/ui.h"
#include "clock.h"
#include "engine_constants.h"
#include "session.h"
#include "ui_constants.h"
#include <stdio.h>
#include <string.h>

static void drawBorder(float x, float y, float w, float h, u32 color) {
    C2D_DrawRectangle(x, y, 0, w, 1, color, color, color, color);         // Top
    C2D_DrawRectangle(x, y + h - 1, 0, w, 1, color, color, color, color); // Bottom
    C2D_DrawRectangle(x, y, 0, 1, h, color, color, color, color);         // Left
    C2D_DrawRectangle(x + w - 1, y, 0, 1, h, color, color, color, color); // Right
}

static void drawClockSettingsCommon(int selected_option, float screen_width) {
    LightLock_Lock(&g_clock_display_lock);
    ClockDisplay clock_display = g_clock_display;
    LightLock_Unlock(&g_clock_display_lock);

    const char *options[]   = { "BPM", "Beats per Bar", "Back" };
    int         num_options = sizeof(options) / sizeof(options[0]);

    // Menu box
    float menu_width  = CLOCK_MENU_WIDTH;
    float menu_height = CLOCK_MENU_HEIGHT;
    float menu_x      = (screen_width - menu_width) / 2;
    float menu_y      = (SCREEN_HEIGHT - menu_height) / 2;
    C2D_DrawRectangle(menu_x, menu_y, 0, menu_width, menu_height, CLR_BLACK, CLR_BLACK, CLR_BLACK,
                      CLR_BLACK);
    // Border
    drawBorder(menu_x, menu_y, menu_width, menu_height, CLR_LIGHT_GRAY);

    for (int i = 0; i < num_options; i++) {
        C2D_Font current_font = (i == selected_option) ? font_heavy : font_angular;
        u32      color        = (i == selected_option) ? CLR_YELLOW : CLR_WHITE;

        C2D_TextBufClear(text_buf);
        char text[64];
        if (i == 0) {
            snprintf(text, sizeof(text), "%s %.0f", options[i], clock_display.bpm);
        } else if (i == 1) {
            snprintf(text, sizeof(text), "%s %d", options[i], clock_display.beats_per_bar);
        } else {
            snprintf(text, sizeof(text), "%s", options[i]);
        }

        C2D_TextFontParse(&text_obj, current_font, text_buf, text);
        C2D_TextOptimize(&text_obj);

        float text_width, text_height;
        C2D_TextGetDimensions(&text_obj, TEXT_SCALE_NORMAL, TEXT_SCALE_NORMAL, &text_width,
                              &text_height);

        float text_x = menu_x + (menu_width - text_width) / 2;
        float text_y = menu_y + 20 + (i * 25);

        C2D_DrawText(&text_obj, C2D_WithColor, text_x, text_y, 0.0f, TEXT_SCALE_NORMAL,
                     TEXT_SCALE_NORMAL, color);
    }
}

void drawClockSettingsView(int selected_option) {
    C2D_DrawRectangle(0, 0, 0, TOP_SCREEN_WIDTH, SCREEN_HEIGHT, C2D_Color32(0, 0, 0, 128),
                      C2D_Color32(0, 0, 0, 128), C2D_Color32(0, 0, 0, 128),
                      C2D_Color32(0, 0, 0, 128));

    drawClockSettingsCommon(selected_option, (float) TOP_SCREEN_WIDTH);
}

void drawQuitMenu(const char *options[], int num_options, int selected_option) {
    // Dim background
    C2D_DrawRectangle(0, 0, 0, TOP_SCREEN_WIDTH, SCREEN_HEIGHT, C2D_Color32(0, 0, 0, 128),
                      C2D_Color32(0, 0, 0, 128), C2D_Color32(0, 0, 0, 128),
                      C2D_Color32(0, 0, 0, 128));

    // Menu box
    float menu_width  = QUIT_MENU_WIDTH;
    float menu_height = QUIT_MENU_HEIGHT;
    float menu_x      = (TOP_SCREEN_WIDTH - menu_width) / 2;
    float menu_y      = (SCREEN_HEIGHT - menu_height) / 2;
    C2D_DrawRectangle(menu_x, menu_y, 0, menu_width, menu_height, CLR_BLACK, CLR_BLACK, CLR_BLACK,
                      CLR_BLACK);
    // Border
    drawBorder(menu_x, menu_y, menu_width, menu_height, CLR_LIGHT_GRAY);

    // Menu options
    for (int i = 0; i < num_options; i++) {
        C2D_Font current_font = (i == selected_option) ? font_heavy : font_angular;
        u32      color        = (i == selected_option) ? CLR_YELLOW : CLR_WHITE;

        C2D_TextBufClear(text_buf);
        C2D_TextFontParse(&text_obj, current_font, text_buf, options[i]);
        C2D_TextOptimize(&text_obj);

        float text_width, text_height;
        C2D_TextGetDimensions(&text_obj, TEXT_SCALE_NORMAL, TEXT_SCALE_NORMAL, &text_width,
                              &text_height);

        float text_x = menu_x + (menu_width - text_width) / 2;
        float text_y = menu_y + 20 + (i * 25);

        C2D_DrawText(&text_obj, C2D_WithColor, text_x, text_y, 0.0f, TEXT_SCALE_NORMAL,
                     TEXT_SCALE_NORMAL, color);
    }
}

void drawTouchScreenSettingsView(int selected_option, ScreenFocus focus) {
    const char *options[]   = { "Clock Settings", "Sample Manager" };
    int         num_options = sizeof(options) / sizeof(options[0]);

    float rect_width  = 150;
    float rect_height = 100;
    float spacing     = 20;
    float total_width = (rect_width * num_options) + (spacing * (num_options - 1));
    float start_x     = (BOTTOM_SCREEN_WIDTH - total_width) / 2;
    float rect_y      = (SCREEN_HEIGHT - rect_height) / 2;

    for (int i = 0; i < num_options; i++) {
        float rect_x = start_x + (rect_width + spacing) * i;
        if (i == selected_option) {
            if (focus == FOCUS_BOTTOM) {
                C2D_DrawRectangle(rect_x, rect_y, 0, rect_width, rect_height, CLR_YELLOW,
                                  CLR_YELLOW, CLR_YELLOW, CLR_YELLOW);
            } else {
                C2D_DrawRectangle(rect_x, rect_y, 0, rect_width, rect_height, CLR_LIGHT_GRAY,
                                  CLR_LIGHT_GRAY, CLR_LIGHT_GRAY, CLR_LIGHT_GRAY);
                u32 border_color = CLR_YELLOW;
                drawBorder(rect_x, rect_y, rect_width, rect_height, border_color);
            }
        } else {
            C2D_DrawRectangle(rect_x, rect_y, 0, rect_width, rect_height, CLR_LIGHT_GRAY,
                              CLR_LIGHT_GRAY, CLR_LIGHT_GRAY, CLR_LIGHT_GRAY);
        }

        C2D_Font current_font = font_angular;
        u32      text_color   = CLR_BLACK;

        C2D_TextBufClear(text_buf);
        C2D_TextFontParse(&text_obj, current_font, text_buf, options[i]);
        C2D_TextOptimize(&text_obj);

        float text_width, text_height;
        C2D_TextGetDimensions(&text_obj, TEXT_SCALE_SMALL, TEXT_SCALE_SMALL, &text_width,
                              &text_height);

        float text_x = rect_x + (rect_width - text_width) / 2;
        float text_y = rect_y + (rect_height - text_height) / 2;

        C2D_DrawText(&text_obj, C2D_WithColor, text_x, text_y, 0.0f, TEXT_SCALE_SMALL,
                     TEXT_SCALE_SMALL, text_color);
    }
}

void drawTouchClockSettingsView(int selected_option) {
    drawClockSettingsCommon(selected_option, (float) BOTTOM_SCREEN_WIDTH);
}

void drawSampleManagerView(SampleBank *bank, int selected_row, int selected_col,
                           bool is_selecting_sample, int selected_sample_browser_index,
                           SampleBrowser *browser, ScreenFocus focus) {
    if (!bank)
        return;
    int   num_rows    = SAMPLE_GRID_ROWS;
    int   num_cols    = SAMPLE_GRID_COLS;
    float cell_width  = BOTTOM_SCREEN_WIDTH / num_cols;
    float cell_height = SCREEN_HEIGHT / num_rows;

    for (int i = 0; i < num_rows; i++) {
        for (int j = 0; j < num_cols; j++) {
            float x = j * cell_width;
            float y = i * cell_height;

            int sample_index = i * num_cols + j;
            if (sample_index < MAX_SAMPLES) {
                char sample_name[64];
                SampleBankGetSampleName(bank, sample_index, sample_name, sizeof(sample_name));
                u32 fill_color =
                    (strcmp(sample_name, "Empty") == 0) ? CLR_DARK_GRAY : CLR_LIGHT_GRAY;
                u32 border_color = fill_color; // Default to fill color

                if (i == selected_row && j == selected_col) {
                    if (focus == FOCUS_BOTTOM) {
                        fill_color = CLR_YELLOW;
                    } else { // FOCUS_TOP
                        border_color = CLR_YELLOW;
                    }
                }

                C2D_TextBufClear(text_buf);
                C2D_TextFontParse(&text_obj, font_angular, text_buf, sample_name);
                C2D_TextOptimize(&text_obj);

                float text_width, text_height;
                C2D_TextGetDimensions(&text_obj, TEXT_SCALE_SMALL, TEXT_SCALE_SMALL, &text_width,
                                      &text_height);

                float text_x = x + (cell_width - text_width) / 2;
                float text_y = y + (cell_height - text_height) / 2;

                C2D_DrawRectangle(x, y, 0, cell_width - 2, cell_height - 2, fill_color, fill_color,
                                  fill_color, fill_color);
                // Draw border if focus is TOP and this is the selected cell
                if (i == selected_row && j == selected_col && focus == FOCUS_TOP) {
                    C2D_DrawRectangle(x, y, 0, cell_width - 2, 1, border_color, border_color,
                                      border_color, border_color); // Top
                    C2D_DrawRectangle(x, y + cell_height - 3, 0, cell_width - 2, 1, border_color,
                                      border_color, border_color, border_color); // Bottom
                    C2D_DrawRectangle(x, y, 0, 1, cell_height - 2, border_color, border_color,
                                      border_color, border_color); // Left
                    C2D_DrawRectangle(x + cell_width - 3, y, 0, 1, cell_height - 2, border_color,
                                      border_color, border_color, border_color); // Right
                }
                C2D_DrawText(&text_obj, C2D_WithColor, text_x, text_y, 0.0f, TEXT_SCALE_SMALL,
                             TEXT_SCALE_SMALL, CLR_BLACK);
            }
        }
    }

    if (is_selecting_sample) {
        if (browser == NULL) {
            return;
        }

        float menu_width  = SAMPLE_BROWSER_WIDTH;
        float menu_height = SAMPLE_BROWSER_HEIGHT;
        float menu_x      = (BOTTOM_SCREEN_WIDTH - menu_width) / 2;
        float menu_y      = (SCREEN_HEIGHT - menu_height) / 2;

        C2D_DrawRectangle(menu_x, menu_y, 0, menu_width, menu_height, CLR_BLACK, CLR_BLACK,
                          CLR_BLACK, CLR_BLACK);
        drawBorder(menu_x, menu_y, menu_width, menu_height, CLR_LIGHT_GRAY);

        int start_index = 0;
        if (selected_sample_browser_index >= 5) {
            start_index = selected_sample_browser_index - 5;
        }

        for (int i = 0; i < SAMPLE_BROWSER_VISIBLE_ITEMS &&
                        (start_index + i) < SampleBrowserGetSampleCount(browser);
             i++) {
            int         index = start_index + i;
            const char *name  = SampleBrowserGetSampleName(browser, index);
            C2D_Font    current_font =
                (index == selected_sample_browser_index) ? font_heavy : font_angular;
            u32 color = (index == selected_sample_browser_index) ? CLR_YELLOW : CLR_WHITE;

            C2D_TextBufClear(text_buf);
            C2D_TextFontParse(&text_obj, current_font, text_buf, name);
            C2D_TextOptimize(&text_obj);

            float text_width, text_height;
            C2D_TextGetDimensions(&text_obj, TEXT_SCALE_SMALL, TEXT_SCALE_SMALL, &text_width,
                                  &text_height);

            float text_x = menu_x + 10;
            float text_y = menu_y + 10 + (i * 14);

            C2D_DrawText(&text_obj, C2D_WithColor, text_x, text_y, 0.0f, TEXT_SCALE_SMALL,
                         TEXT_SCALE_SMALL, color);
        }
    }
}