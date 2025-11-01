#include "views.h"

#include "audio_utils.h"
#include "engine_constants.h"
#include "filters.h"
#include "polybleposc.h"
#include "samplers.h"
#include "sample.h"
#include "sample_bank.h"
#include "session_controller.h"
#include "ui_constants.h"
#include <stdio.h>

static C2D_Font    font_angular;
static C2D_Font    font_heavy;
static C2D_TextBuf text_buf;
static C2D_Text    text_obj;

void initViews() {
    font_angular = C2D_FontLoad(FONTPATH_F500ANGULAR);
    font_heavy   = C2D_FontLoad(FONTPATH_2197HEAVY);
    text_buf     = C2D_TextBufNew(128);
}

void deinitViews() {
    C2D_FontFree(font_angular);
    C2D_FontFree(font_heavy);
    C2D_TextBufDelete(text_buf);
}

void drawStepsBar(int cur_step) {
    for (int i = 0; i < 16; i++) {
        u32 color = (i == cur_step) ? CLR_RED : CLR_DARK_GRAY;
        C2D_DrawRectangle(HOME_TRACKS_WIDTH + HOME_STEPS_SPACER_W * (i + 1) +
                              HOME_STEPS_HEADER_W * i,
                          0, 0, HOME_STEPS_HEADER_W, HOME_STEPS_HEIGHT, color, color, color, color);
    }
}

static const char *get_status_symbol(ClockStatus status) {
    switch (status) {
    case PLAYING:
        return "P";
    case STOPPED:
        return "S";
    case PAUSED:
        return "| |";
    default:
        return "";
    }
}

void drawTrackbar(Clock *clock, Track *tracks) {
    float track_height = SCREEN_HEIGHT / 13;
    for (int i = 0; i < N_TRACKS + 1; i++) {
        if (i == 0) {
            // Draw outline for the header box
            C2D_DrawRectangle(0, 0, 0, HOME_TRACKS_WIDTH, 1, CLR_LIGHT_GRAY, CLR_LIGHT_GRAY,
                              CLR_LIGHT_GRAY, CLR_LIGHT_GRAY);
            C2D_DrawRectangle(0, track_height - 3, 0, HOME_TRACKS_WIDTH, 1, CLR_LIGHT_GRAY,
                              CLR_LIGHT_GRAY, CLR_LIGHT_GRAY, CLR_LIGHT_GRAY);
            C2D_DrawRectangle(0, 0, 0, 1, track_height - 2, CLR_LIGHT_GRAY, CLR_LIGHT_GRAY,
                              CLR_LIGHT_GRAY, CLR_LIGHT_GRAY);
            C2D_DrawRectangle(HOME_TRACKS_WIDTH - 1, 0, 0, 1, track_height - 2, CLR_LIGHT_GRAY,
                              CLR_LIGHT_GRAY, CLR_LIGHT_GRAY, CLR_LIGHT_GRAY);

            // Draw bar.beat text
            C2D_TextBufClear(text_buf);
            char buf[64];
            snprintf(buf, sizeof(buf), "%d.%d", clock->barBeats->bar, clock->barBeats->beat + 1);
            C2D_TextFontParse(&text_obj, font_angular, text_buf, buf);
            C2D_TextOptimize(&text_obj);

            float text_width, text_height;
            C2D_TextGetDimensions(&text_obj, 0.4f, 0.4f, &text_width, &text_height);

            float text_x = (HOME_TRACKS_WIDTH / 2.0f);
            float text_y = (track_height - text_height) / 2.0f;

            C2D_DrawText(&text_obj, C2D_WithColor | C2D_AlignCenter, text_x / 2, text_y, 0.0f, 0.4f,
                         0.4f, CLR_LIGHT_GRAY);

            // Draw bpm status text
            C2D_TextBufClear(text_buf);
            snprintf(buf, sizeof(buf), "%.0f %s", clock->bpm, get_status_symbol(clock->status));
            C2D_TextFontParse(&text_obj, font_angular, text_buf, buf);
            C2D_TextOptimize(&text_obj);

            C2D_TextGetDimensions(&text_obj, 0.4f, 0.4f, &text_width, &text_height);
            text_x = (HOME_TRACKS_WIDTH / 2.0f) + (HOME_TRACKS_WIDTH / 2.0f) / 2.0f;
            text_y = (track_height - text_height) / 2.0f;

            C2D_DrawText(&text_obj, C2D_WithColor | C2D_AlignCenter, text_x, text_y, 0.0f, 0.4f,
                         0.4f, CLR_LIGHT_GRAY);

        } else {
            C2D_DrawRectangle(0, i * track_height, 0, HOME_TRACKS_WIDTH,
                              track_height - 2, // -2 for spacing
                              CLR_LIGHT_GRAY, CLR_LIGHT_GRAY, CLR_LIGHT_GRAY, CLR_LIGHT_GRAY);

            int         track_idx       = i - 1;
            const char *instrument_name = "";
            if (tracks[track_idx].instrument_type == SUB_SYNTH) {
                instrument_name = "Synth";
            } else if (tracks[track_idx].instrument_type == OPUS_SAMPLER) {
                instrument_name = "Sampler";
            }

            C2D_TextBufClear(text_buf);
            C2D_TextFontParse(&text_obj, font_angular, text_buf, instrument_name);
            C2D_TextOptimize(&text_obj);

            float text_width, text_height;
            C2D_TextGetDimensions(&text_obj, 0.4f, 0.4f, &text_width, &text_height);

            float text_x = HOME_TRACKS_WIDTH / 2.0f;
            float text_y = (i * track_height) + (track_height - text_height) / 2.0f;

            C2D_DrawText(&text_obj, C2D_WithColor | C2D_AlignCenter, text_x, text_y, 0.0f, 0.4f,
                         0.4f, CLR_BLACK);
        }
    }
}

void drawTracksSequencers(Track *tracks, int cur_step) {
    float track_height = SCREEN_HEIGHT / 13;
    for (int i = 0; i < N_TRACKS; i++) {
        if (tracks[i].sequencer) {
            for (int j = 0; j < 16; j++) {
                u32   color = tracks[i].sequencer->steps[j].active ? CLR_LIGHT_GRAY : CLR_DARK_GRAY;
                float x =
                    HOME_TRACKS_WIDTH + HOME_STEPS_SPACER_W * (j + 1) + HOME_STEPS_HEADER_W * j;
                float y = (i + 1) * track_height;
                float w = HOME_STEPS_HEADER_W;
                float h = track_height - 2;

                if (j == cur_step) {
                    C2D_DrawRectangle(x - 1, y - 1, 0, w + 2, h + 2, CLR_RED, CLR_RED, CLR_RED,
                                      CLR_RED);
                }

                C2D_DrawRectangle(x, y, 0, w, h, color, color, color, color);
            }
        }
    }
}

static void drawSelectionOverlay(int row, int col, bool is_focused) {
    float track_height = SCREEN_HEIGHT / 13;
    float x, y, w, h;

    if (col == 0) { // Track info column
        x = 0;
        y = row * track_height;
        w = HOME_TRACKS_WIDTH;
        h = track_height - 2;
    } else { // Sequencer step column
        int j = col - 1;
        x     = HOME_TRACKS_WIDTH + HOME_STEPS_SPACER_W * (j + 1) + HOME_STEPS_HEADER_W * j;
        w     = HOME_STEPS_HEADER_W;

        if (row == 0) { // Header row for sequencer
            y = 0;
            h = (N_TRACKS + 1) * track_height;
        } else {
            y = row * track_height;
            h = track_height - 2;
        }
    }

    if (is_focused) {
        C2D_DrawRectangle(x, y, 0, w, h, CLR_YELLOW, CLR_YELLOW, CLR_YELLOW, CLR_YELLOW);
    } else {
        u32 border_color = CLR_YELLOW;
        C2D_DrawRectangle(x, y, 0, w, 1, border_color, border_color, border_color,
                          border_color); // Top
        C2D_DrawRectangle(x, y + h - 1, 0, w, 1, border_color, border_color, border_color,
                          border_color); // Bottom
        C2D_DrawRectangle(x, y, 0, 1, h, border_color, border_color, border_color,
                          border_color); // Left
        C2D_DrawRectangle(x + w - 1, y, 0, 1, h, border_color, border_color, border_color,
                          border_color); // Right
    }
}

void drawMainView(Track *tracks, Clock *clock, int selected_row, int selected_col,
                  ScreenFocus focus) {
    int cur_step = -1; // Default to no active step
    if (tracks && tracks[0].sequencer) {
        int steps_per_beat = tracks[0].sequencer->steps_per_beat;
        int total_steps    = tracks[0].sequencer->n_beats * steps_per_beat;

        if (clock->status == PLAYING || clock->status == PAUSED) {
            if (steps_per_beat > 0) {
                int clock_steps_per_seq_step = STEPS_PER_BEAT / steps_per_beat;
                if (clock_steps_per_seq_step > 0 && total_steps > 0) {
                    cur_step = ((clock->barBeats->steps > 0 ? clock->barBeats->steps - 1 : 0) /
                                clock_steps_per_seq_step) %
                               total_steps;
                }
            }
        } else { // STOPPED
            cur_step = tracks[0].sequencer->cur_step;
        }
    }

    drawStepsBar(cur_step);
    drawTrackbar(clock, tracks);
    drawTracksSequencers(tracks, cur_step);
    drawSelectionOverlay(selected_row, selected_col, focus == FOCUS_TOP);
}

void drawClockSettingsView(Clock *clock, int selected_option) {
    C2D_DrawRectangle(0, 0, 0, TOP_SCREEN_WIDTH, SCREEN_HEIGHT, C2D_Color32(0, 0, 0, 128),
                      C2D_Color32(0, 0, 0, 128), C2D_Color32(0, 0, 0, 128),
                      C2D_Color32(0, 0, 0, 128));
    const char *options[]   = { "BPM", "Beats per Bar", "Back" };
    int         num_options = sizeof(options) / sizeof(options[0]);

    // Menu box
    float menu_width  = 300;
    float menu_height = 100;
    float menu_x      = (TOP_SCREEN_WIDTH - menu_width) / 2;
    float menu_y      = (SCREEN_HEIGHT - menu_height) / 2;
    C2D_DrawRectangle(menu_x, menu_y, 0, menu_width, menu_height, CLR_BLACK, CLR_BLACK, CLR_BLACK,
                      CLR_BLACK);
    // Border
    C2D_DrawRectangle(menu_x, menu_y, 0, menu_width, 1, CLR_LIGHT_GRAY, CLR_LIGHT_GRAY,
                      CLR_LIGHT_GRAY, CLR_LIGHT_GRAY);
    C2D_DrawRectangle(menu_x, menu_y + menu_height - 1, 0, menu_width, 1, CLR_LIGHT_GRAY,
                      CLR_LIGHT_GRAY, CLR_LIGHT_GRAY, CLR_LIGHT_GRAY);
    C2D_DrawRectangle(menu_x, menu_y, 0, 1, menu_height, CLR_LIGHT_GRAY, CLR_LIGHT_GRAY,
                      CLR_LIGHT_GRAY, CLR_LIGHT_GRAY);
    C2D_DrawRectangle(menu_x + menu_width - 1, menu_y, 0, 1, menu_height, CLR_LIGHT_GRAY,
                      CLR_LIGHT_GRAY, CLR_LIGHT_GRAY, CLR_LIGHT_GRAY);

    for (int i = 0; i < num_options; i++) {
        C2D_Font current_font = (i == selected_option) ? font_heavy : font_angular;
        u32      color        = (i == selected_option) ? CLR_YELLOW : CLR_WHITE;

        C2D_TextBufClear(text_buf);
        char text[64];
        if (i == 0) {
            snprintf(text, sizeof(text), "%s %.0f", options[i], clock->bpm);
        } else if (i == 1) {
            snprintf(text, sizeof(text), "%s %d", options[i], clock->barBeats->beats_per_bar);
        } else {
            snprintf(text, sizeof(text), "%s", options[i]);
        }

        C2D_TextFontParse(&text_obj, current_font, text_buf, text);
        C2D_TextOptimize(&text_obj);

        float text_width, text_height;
        C2D_TextGetDimensions(&text_obj, 0.5f, 0.5f, &text_width, &text_height);

        float text_x = menu_x + (menu_width - text_width) / 2;
        float text_y = menu_y + 20 + (i * 25);

        C2D_DrawText(&text_obj, C2D_WithColor, text_x, text_y, 0.0f, 0.5f, 0.5f, color);
    }
}

void drawQuitMenu(const char *options[], int num_options, int selected_option) {
    // Dim background
    C2D_DrawRectangle(0, 0, 0, TOP_SCREEN_WIDTH, SCREEN_HEIGHT, C2D_Color32(0, 0, 0, 128),
                      C2D_Color32(0, 0, 0, 128), C2D_Color32(0, 0, 0, 128),
                      C2D_Color32(0, 0, 0, 128));

    // Menu box
    float menu_width  = 150;
    float menu_height = 80;
    float menu_x      = (TOP_SCREEN_WIDTH - menu_width) / 2;
    float menu_y      = (SCREEN_HEIGHT - menu_height) / 2;
    C2D_DrawRectangle(menu_x, menu_y, 0, menu_width, menu_height, CLR_BLACK, CLR_BLACK, CLR_BLACK,
                      CLR_BLACK);
    // Border
    C2D_DrawRectangle(menu_x, menu_y, 0, menu_width, 1, CLR_LIGHT_GRAY, CLR_LIGHT_GRAY,
                      CLR_LIGHT_GRAY, CLR_LIGHT_GRAY);
    C2D_DrawRectangle(menu_x, menu_y + menu_height - 1, 0, menu_width, 1, CLR_LIGHT_GRAY,
                      CLR_LIGHT_GRAY, CLR_LIGHT_GRAY, CLR_LIGHT_GRAY);
    C2D_DrawRectangle(menu_x, menu_y, 0, 1, menu_height, CLR_LIGHT_GRAY, CLR_LIGHT_GRAY,
                      CLR_LIGHT_GRAY, CLR_LIGHT_GRAY);
    C2D_DrawRectangle(menu_x + menu_width - 1, menu_y, 0, 1, menu_height, CLR_LIGHT_GRAY,
                      CLR_LIGHT_GRAY, CLR_LIGHT_GRAY, CLR_LIGHT_GRAY);

    // Menu options
    for (int i = 0; i < num_options; i++) {
        C2D_Font current_font = (i == selected_option) ? font_heavy : font_angular;
        u32      color        = (i == selected_option) ? CLR_YELLOW : CLR_WHITE;

        C2D_TextBufClear(text_buf);
        C2D_TextFontParse(&text_obj, current_font, text_buf, options[i]);
        C2D_TextOptimize(&text_obj);

        float text_width, text_height;
        C2D_TextGetDimensions(&text_obj, 0.5f, 0.5f, &text_width, &text_height);

        float text_x = menu_x + (menu_width - text_width) / 2;
        float text_y = menu_y + 20 + (i * 25);

        C2D_DrawText(&text_obj, C2D_WithColor, text_x, text_y, 0.0f, 0.5f, 0.5f, color);
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
                C2D_DrawRectangle(rect_x, rect_y, 0, rect_width, 1, border_color, border_color,
                                  border_color, border_color); // Top
                C2D_DrawRectangle(rect_x, rect_y + rect_height - 1, 0, rect_width, 1, border_color,
                                  border_color, border_color, border_color); // Bottom
                C2D_DrawRectangle(rect_x, rect_y, 0, 1, rect_height, border_color, border_color,
                                  border_color, border_color); // Left
                C2D_DrawRectangle(rect_x + rect_width - 1, rect_y, 0, 1, rect_height, border_color,
                                  border_color, border_color, border_color); // Right
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
        C2D_TextGetDimensions(&text_obj, 0.4f, 0.4f, &text_width, &text_height);

        float text_x = rect_x + (rect_width - text_width) / 2;
        float text_y = rect_y + (rect_height - text_height) / 2;

        C2D_DrawText(&text_obj, C2D_WithColor, text_x, text_y, 0.0f, 0.4f, 0.4f, text_color);
    }
}

void drawTouchClockSettingsView(Clock *clock, int selected_option) {
    const char *options[]   = { "BPM", "Beats per Bar", "Back" };
    int         num_options = sizeof(options) / sizeof(options[0]);

    // Menu box
    float menu_width  = 300;
    float menu_height = 100;
    float menu_x      = (BOTTOM_SCREEN_WIDTH - menu_width) / 2;
    float menu_y      = (SCREEN_HEIGHT - menu_height) / 2;
    C2D_DrawRectangle(menu_x, menu_y, 0, menu_width, menu_height, CLR_BLACK, CLR_BLACK, CLR_BLACK,
                      CLR_BLACK);
    // Border
    C2D_DrawRectangle(menu_x, menu_y, 0, menu_width, 1, CLR_LIGHT_GRAY, CLR_LIGHT_GRAY,
                      CLR_LIGHT_GRAY, CLR_LIGHT_GRAY);
    C2D_DrawRectangle(menu_x, menu_y + menu_height - 1, 0, menu_width, 1, CLR_LIGHT_GRAY,
                      CLR_LIGHT_GRAY, CLR_LIGHT_GRAY, CLR_LIGHT_GRAY);
    C2D_DrawRectangle(menu_x, menu_y, 0, 1, menu_height, CLR_LIGHT_GRAY, CLR_LIGHT_GRAY,
                      CLR_LIGHT_GRAY, CLR_LIGHT_GRAY);
    C2D_DrawRectangle(menu_x + menu_width - 1, menu_y, 0, 1, menu_height, CLR_LIGHT_GRAY,
                      CLR_LIGHT_GRAY, CLR_LIGHT_GRAY, CLR_LIGHT_GRAY);

    for (int i = 0; i < num_options; i++) {
        C2D_Font current_font = (i == selected_option) ? font_heavy : font_angular;
        u32      color        = (i == selected_option) ? CLR_YELLOW : CLR_WHITE;

        C2D_TextBufClear(text_buf);
        char text[64];
        if (i == 0) {
            snprintf(text, sizeof(text), "%s %.0f", options[i], clock->bpm);
        } else if (i == 1) {
            snprintf(text, sizeof(text), "%s %d", options[i], clock->barBeats->beats_per_bar);
        } else {
            snprintf(text, sizeof(text), "%s", options[i]);
        }

        C2D_TextFontParse(&text_obj, current_font, text_buf, text);
        C2D_TextOptimize(&text_obj);

        float text_width, text_height;
        C2D_TextGetDimensions(&text_obj, 0.5f, 0.5f, &text_width, &text_height);

        float text_x = menu_x + (menu_width - text_width) / 2;
        float text_y = menu_y + 20 + (i * 25);

        C2D_DrawText(&text_obj, C2D_WithColor, text_x, text_y, 0.0f, 0.5f, 0.5f, color);
    }
}
void drawSampleManagerView(SampleBank *bank, int selected_row, int selected_col) {
    int   num_rows    = 3;
    int   num_cols    = 4;
    float cell_width  = BOTTOM_SCREEN_WIDTH / num_cols;
    float cell_height = SCREEN_HEIGHT / num_rows;

    for (int i = 0; i < num_rows; i++) {
        for (int j = 0; j < num_cols; j++) {
            float x     = j * cell_width;
            float y     = i * cell_height;
            u32   color = (i == selected_row && j == selected_col) ? CLR_YELLOW : CLR_DARK_GRAY;
            C2D_DrawRectangle(x, y, 0, cell_width - 2, cell_height - 2, color, color, color, color);

            int sample_index = i * num_cols + j;
            if (sample_index < MAX_SAMPLES) {
                const char *sample_name = SampleBank_get_sample_name(bank, sample_index);
                C2D_TextBufClear(text_buf);
                C2D_TextFontParse(&text_obj, font_angular, text_buf, sample_name);
                C2D_TextOptimize(&text_obj);

                float text_width, text_height;
                C2D_TextGetDimensions(&text_obj, 0.4f, 0.4f, &text_width, &text_height);

                float text_x = x + (cell_width - text_width) / 2;
                float text_y = y + (cell_height - text_height) / 2;

                C2D_DrawText(&text_obj, C2D_WithColor, text_x, text_y, 0.0f, 0.4f, 0.4f, CLR_BLACK);
            }
        }
    }
}

void drawStepSettingsView(Session *session, Track *tracks, int selected_row, int selected_col,
                          int selected_step_option, SampleBank *sample_bank, ScreenFocus focus) {
    if (selected_row == 0 || selected_col == 0) {
        C2D_TextBufClear(text_buf);
        C2D_TextFontParse(&text_obj, font_angular, text_buf, "No Track Selected");
        C2D_TextOptimize(&text_obj);

        float text_width, text_height;
        C2D_TextGetDimensions(&text_obj, 0.5f, 0.5f, &text_width, &text_height);

        float text_x = (BOTTOM_SCREEN_WIDTH - text_width) / 2;
        float text_y = (SCREEN_HEIGHT - text_height) / 2;

        C2D_DrawText(&text_obj, C2D_WithColor, text_x, text_y, 0.0f, 0.5f, 0.5f, CLR_LIGHT_GRAY);
    } else {
        int     track_idx = selected_row - 1;
        int     step_idx  = selected_col - 1;
        Track   track     = tracks[track_idx];
        SeqStep seq_step  = track.sequencer->steps[step_idx];

        const char *instrument_name = "";
        if (track.instrument_type == SUB_SYNTH) {
            instrument_name = "Synth";
        } else if (track.instrument_type == OPUS_SAMPLER) {
            instrument_name = "Sampler";
        }

        C2D_TextBufClear(text_buf);
        C2D_TextFontParse(&text_obj, font_angular, text_buf, instrument_name);
        C2D_TextOptimize(&text_obj);
        C2D_DrawText(&text_obj, C2D_WithColor, 10, 10, 0.0f, 0.5f, 0.5f, CLR_LIGHT_GRAY);

        char  buffer[128];
        float cell_width  = 140;
        float cell_height = 20;
        float padding     = 5;

        u32 base_bg_color, base_text_color, border_color;
        if (seq_step.active) {
            base_bg_color   = CLR_LIGHT_GRAY;
            base_text_color = CLR_BLACK;
            border_color    = CLR_BLACK;
        } else {
            base_bg_color   = CLR_BLACK;
            base_text_color = CLR_LIGHT_GRAY;
            border_color    = CLR_LIGHT_GRAY;
        }

        // Common Parameters
        const char *common_params[] = { "Volume", "Pan", "Filter Cf", "Filter Type" };
        for (int i = 0; i < 4; i++) {
            float x           = 10;
            float y           = 30 + i * (cell_height + padding);
            bool  is_selected = (selected_step_option == i);
            u32   bg_color, text_color;

            if (is_selected) {
                if (focus == FOCUS_BOTTOM) {
                    bg_color   = CLR_YELLOW;
                    text_color = CLR_BLACK;
                } else {
                    bg_color   = base_bg_color;
                    text_color = base_text_color;
                }
            } else {
                bg_color   = base_bg_color;
                text_color = base_text_color;
            }

            C2D_DrawRectangle(x, y, 0, cell_width, cell_height, bg_color, bg_color, bg_color,
                              bg_color);

            u32 current_border_color =
                (is_selected && focus != FOCUS_BOTTOM) ? CLR_YELLOW : border_color;
            C2D_DrawRectangle(x, y, 0, cell_width, 1, current_border_color, current_border_color,
                              current_border_color, current_border_color);
            C2D_DrawRectangle(x, y + cell_height - 1, 0, cell_width, 1, current_border_color,
                              current_border_color, current_border_color, current_border_color);
            C2D_DrawRectangle(x, y, 0, 1, cell_height, current_border_color, current_border_color,
                              current_border_color, current_border_color);
            C2D_DrawRectangle(x + cell_width - 1, y, 0, 1, cell_height, current_border_color,
                              current_border_color, current_border_color, current_border_color);

            C2D_TextBufClear(text_buf);
            switch (i) {
            case 0:
                snprintf(buffer, sizeof(buffer), "%s: %.2f", common_params[i],
                         seq_step.data->volume);
                break;
            case 1:
                snprintf(buffer, sizeof(buffer), "%s: %.2f", common_params[i], seq_step.data->pan);
                break;
            case 2:
                snprintf(buffer, sizeof(buffer), "%s: %.0f", common_params[i],
                         seq_step.data->ndsp_filter_cutoff);
                break;
            case 3:
                snprintf(buffer, sizeof(buffer), "%s: %s", common_params[i],
                         ndsp_biquad_filter_names[seq_step.data->ndsp_filter_type]);
                break;
            }
            C2D_TextFontParse(&text_obj, font_angular, text_buf, buffer);
            C2D_TextOptimize(&text_obj);
            C2D_DrawText(&text_obj, C2D_WithColor, x + padding, y + padding, 0.0f, 0.3f, 0.3f,
                         text_color);
        }

        // Instrument-specific Parameters
        if (track.instrument_type == SUB_SYNTH) {
            SubSynthParameters *params = (SubSynthParameters *) seq_step.data->instrument_data;
            const char         *synth_params[] = { "MIDI Note", "Waveform" };
            for (int i = 0; i < 2; i++) {
                float x           = 160;
                float y           = 30 + i * (cell_height + padding);
                bool  is_selected = (selected_step_option == i + 4);
                u32   bg_color, text_color;

                if (is_selected) {
                    if (focus == FOCUS_BOTTOM) {
                        bg_color   = CLR_YELLOW;
                        text_color = CLR_BLACK;
                    } else {
                        bg_color   = base_bg_color;
                        text_color = base_text_color;
                    }
                } else {
                    bg_color   = base_bg_color;
                    text_color = base_text_color;
                }

                C2D_DrawRectangle(x, y, 0, cell_width, cell_height, bg_color, bg_color, bg_color,
                                  bg_color);

                u32 current_border_color =
                    (is_selected && focus != FOCUS_BOTTOM) ? CLR_YELLOW : border_color;
                C2D_DrawRectangle(x, y, 0, cell_width, 1, current_border_color,
                                  current_border_color, current_border_color, current_border_color);
                C2D_DrawRectangle(x, y + cell_height - 1, 0, cell_width, 1, current_border_color,
                                  current_border_color, current_border_color, current_border_color);
                C2D_DrawRectangle(x, y, 0, 1, cell_height, current_border_color,
                                  current_border_color, current_border_color, current_border_color);
                C2D_DrawRectangle(x + cell_width - 1, y, 0, 1, cell_height, current_border_color,
                                  current_border_color, current_border_color, current_border_color);

                C2D_TextBufClear(text_buf);
                switch (i) {
                case 0: {
                    int midi_note = hertzToMidi(params->osc_freq);
                    snprintf(buffer, sizeof(buffer), "%s: %d", synth_params[i], midi_note);
                    break;
                }
                case 1:
                    snprintf(buffer, sizeof(buffer), "%s: %s", synth_params[i],
                             waveform_names[params->osc_waveform]);
                    break;
                }
                C2D_TextFontParse(&text_obj, font_angular, text_buf, buffer);
                C2D_TextOptimize(&text_obj);
                C2D_DrawText(&text_obj, C2D_WithColor, x + padding, y + padding, 0.0f, 0.3f, 0.3f,
                             text_color);
            }
        } else if (track.instrument_type == OPUS_SAMPLER) {
            OpusSamplerParameters *params =
                (OpusSamplerParameters *) seq_step.data->instrument_data;
            const char *sampler_params[]      = { "Sample", "Looping", "Start Pos" };
            const char *playback_mode_names[] = { "One Shot", "Loop" };
            for (int i = 0; i < 3; i++) {
                float x           = 160;
                float y           = 30 + i * (cell_height + padding);
                bool  is_selected = (selected_step_option == i + 4);
                u32   bg_color, text_color;

                if (is_selected) {
                    if (focus == FOCUS_BOTTOM) {
                        bg_color   = CLR_YELLOW;
                        text_color = CLR_BLACK;
                    } else {
                        bg_color   = base_bg_color;
                        text_color = base_text_color;
                    }
                } else {
                    bg_color   = base_bg_color;
                    text_color = base_text_color;
                }

                C2D_DrawRectangle(x, y, 0, cell_width, cell_height, bg_color, bg_color, bg_color,
                                  bg_color);

                u32 current_border_color =
                    (is_selected && focus != FOCUS_BOTTOM) ? CLR_YELLOW : border_color;
                C2D_DrawRectangle(x, y, 0, cell_width, 1, current_border_color,
                                  current_border_color, current_border_color, current_border_color);
                C2D_DrawRectangle(x, y + cell_height - 1, 0, cell_width, 1, current_border_color,
                                  current_border_color, current_border_color, current_border_color);
                C2D_DrawRectangle(x, y, 0, 1, cell_height, current_border_color,
                                  current_border_color, current_border_color, current_border_color);
                C2D_DrawRectangle(x + cell_width - 1, y, 0, 1, cell_height, current_border_color,
                                  current_border_color, current_border_color, current_border_color);

                C2D_TextBufClear(text_buf);
                switch (i) {
                case 0:
                    snprintf(buffer, sizeof(buffer), "%s: %s", sampler_params[i],
                             SampleBank_get_sample_name(sample_bank, params->sample_index));
                    break;
                case 1:
                    snprintf(buffer, sizeof(buffer), "%s: %s", sampler_params[i],
                             playback_mode_names[params->playback_mode]);
                    break;
                case 2:
                    snprintf(buffer, sizeof(buffer), "%s: %lld", sampler_params[i],
                             params->start_position);
                    break;
                }
                C2D_TextFontParse(&text_obj, font_angular, text_buf, buffer);
                C2D_TextOptimize(&text_obj);
                C2D_DrawText(&text_obj, C2D_WithColor, x + padding, y + padding, 0.0f, 0.3f, 0.3f,
                             text_color);
            }
        }

        // Track and Step Info
        snprintf(buffer, sizeof(buffer), "Tr: %d | St: %d", track_idx + 1, step_idx + 1);
        C2D_TextBufClear(text_buf);
        C2D_TextFontParse(&text_obj, font_angular, text_buf, buffer);
        C2D_TextOptimize(&text_obj);
        C2D_DrawText(&text_obj, C2D_WithColor, 200, 10, 0.0f, 0.4f, 0.4f, CLR_LIGHT_GRAY);
    }
}

void drawStepSettingsEditView(Track *track, TrackParameters *params, int selected_step_option,
                              SampleBank *sample_bank) {
    C2D_DrawRectangle(0, 0, 0, TOP_SCREEN_WIDTH, SCREEN_HEIGHT, C2D_Color32(0, 0, 0, 128),
                      C2D_Color32(0, 0, 0, 128), C2D_Color32(0, 0, 0, 128),
                      C2D_Color32(0, 0, 0, 128));

    // Menu box
    float menu_width  = 300;
    float menu_height = 100;
    float menu_x      = (TOP_SCREEN_WIDTH - menu_width) / 2;
    float menu_y      = (SCREEN_HEIGHT - menu_height) / 2;
    C2D_DrawRectangle(menu_x, menu_y, 0, menu_width, menu_height, CLR_BLACK, CLR_BLACK, CLR_BLACK,
                      CLR_BLACK);
    // Border
    C2D_DrawRectangle(menu_x, menu_y, 0, menu_width, 1, CLR_LIGHT_GRAY, CLR_LIGHT_GRAY,
                      CLR_LIGHT_GRAY, CLR_LIGHT_GRAY);
    C2D_DrawRectangle(menu_x, menu_y + menu_height - 1, 0, menu_width, 1, CLR_LIGHT_GRAY,
                      CLR_LIGHT_GRAY, CLR_LIGHT_GRAY, CLR_LIGHT_GRAY);
    C2D_DrawRectangle(menu_x, menu_y, 0, 1, menu_height, CLR_LIGHT_GRAY, CLR_LIGHT_GRAY,
                      CLR_LIGHT_GRAY, CLR_LIGHT_GRAY);
    C2D_DrawRectangle(menu_x + menu_width - 1, menu_y, 0, 1, menu_height, CLR_LIGHT_GRAY,
                      CLR_LIGHT_GRAY, CLR_LIGHT_GRAY, CLR_LIGHT_GRAY);

    C2D_TextBufClear(text_buf);
    char text[64];

    if (selected_step_option == 0) { // Volume
        snprintf(text, sizeof(text), "Volume: %.1f", params->volume);
    } else if (selected_step_option == 1) { // Pan
        snprintf(text, sizeof(text), "Pan: %.1f", params->pan);
    } else if (selected_step_option == 2) { // Filter CF
        snprintf(text, sizeof(text), "Filter Cf: %.0f", params->ndsp_filter_cutoff);
    } else if (selected_step_option == 3) { // Filter Type
        snprintf(text, sizeof(text), "Filter Type: %s",
                 ndsp_biquad_filter_names[params->ndsp_filter_type]);
    } else if (track->instrument_type == SUB_SYNTH) {
        SubSynthParameters *synth_params = (SubSynthParameters *) params->instrument_data;
        if (selected_step_option == 4) { // MIDI Note
            int midi_note = hertzToMidi(synth_params->osc_freq);
            snprintf(text, sizeof(text), "MIDI Note: %d", midi_note);
        } else if (selected_step_option == 5) { // Waveform
            snprintf(text, sizeof(text), "Waveform: %s",
                     waveform_names[synth_params->osc_waveform]);
        }
    } else if (track->instrument_type == OPUS_SAMPLER) {
        OpusSamplerParameters *sampler_params = (OpusSamplerParameters *) params->instrument_data;
        if (selected_step_option == 4) { // Sample
            snprintf(text, sizeof(text), "Sample: %s",
                     SampleBank_get_sample_name(sample_bank, sampler_params->sample_index));
        }
    }

    C2D_TextFontParse(&text_obj, font_heavy, text_buf, text);
    C2D_TextOptimize(&text_obj);

    float text_width, text_height;
    C2D_TextGetDimensions(&text_obj, 0.5f, 0.5f, &text_width, &text_height);

    float text_x = menu_x + (menu_width - text_width) / 2;
    float text_y = menu_y + (menu_height - text_height) / 2;

    C2D_DrawText(&text_obj, C2D_WithColor, text_x, text_y, 0.0f, 0.5f, 0.5f, CLR_YELLOW);
}