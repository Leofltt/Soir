#include "ui/ui.h"
#include "clock.h"
#include "engine_constants.h"
#include "session.h"
#include "ui_constants.h"
#include <citro2d.h>
#include <stdio.h>
#include <string.h>

extern C2D_Font    font_angular;
extern C2D_Font    font_heavy;
extern C2D_TextBuf text_buf;
extern C2D_Text    text_obj;

void drawStepsBar(int cur_step, int steps_per_beat) {
    for (int i = 0; i < 16; i++) {
        u32 color = (i == cur_step) ? CLR_RED : CLR_DARK_GRAY;
        C2D_DrawRectangle(HOME_TRACKS_WIDTH + HOME_STEPS_SPACER_W * (i + 1) +
                              HOME_STEPS_HEADER_W * i,
                          0, 0, HOME_STEPS_HEADER_W, HOME_STEPS_HEIGHT, color, color, color, color);

        // Draw beat indicator
        if (steps_per_beat > 0 && i % steps_per_beat == 0) {
            u32 indicator_color =
                (i == cur_step) ? CLR_DARK_GRAY : CLR_BLACK; // Invert color when active
            float rect_x =
                HOME_TRACKS_WIDTH + HOME_STEPS_SPACER_W * (i + 1) + HOME_STEPS_HEADER_W * i;
            float rect_y = 0;
            float rect_w = HOME_STEPS_HEADER_W;
            float rect_h = HOME_STEPS_HEIGHT;

            // Calculate position and size for the smaller square (e.g., 1/3 of the main square
            // size, centered)
            float indicator_w = rect_w / 3.0f;
            float indicator_h = rect_h / 3.0f;
            float indicator_x = rect_x + (rect_w - indicator_w) / 2.0f;
            float indicator_y = rect_y + (rect_h - indicator_h) / 2.0f;

            C2D_DrawRectangle(indicator_x, indicator_y, 0, indicator_w, indicator_h,
                              indicator_color, indicator_color, indicator_color, indicator_color);
        }
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

void drawTrackbar(Track *tracks) {
    LightLock_Lock(&g_clock_display_lock);
    ClockDisplay clock_display = g_clock_display;
    LightLock_Unlock(&g_clock_display_lock);

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
            snprintf(buf, sizeof(buf), "%d.%d", clock_display.bar, clock_display.beat + 1);
            C2D_TextFontParse(&text_obj, font_angular, text_buf, buf);
            C2D_TextOptimize(&text_obj);

            float text_width, text_height;
            C2D_TextGetDimensions(&text_obj, TEXT_SCALE_SMALL, TEXT_SCALE_SMALL, &text_width,
                                  &text_height);

            float text_x = (HOME_TRACKS_WIDTH / 2.0f);
            float text_y = (track_height - text_height) / 2.0f;

            C2D_DrawText(&text_obj, C2D_WithColor | C2D_AlignCenter, text_x / 2, text_y, 0.0f,
                         TEXT_SCALE_SMALL, TEXT_SCALE_SMALL, CLR_LIGHT_GRAY);

            // Draw bpm status text
            C2D_TextBufClear(text_buf);
            snprintf(buf, sizeof(buf), "%.0f %s", clock_display.bpm,
                     get_status_symbol(clock_display.status));
            C2D_TextFontParse(&text_obj, font_angular, text_buf, buf);
            C2D_TextOptimize(&text_obj);

            C2D_TextGetDimensions(&text_obj, TEXT_SCALE_SMALL, TEXT_SCALE_SMALL, &text_width,
                                  &text_height);
            text_x = (HOME_TRACKS_WIDTH / 2.0f) + (HOME_TRACKS_WIDTH / 2.0f) / 2.0f;
            text_y = (track_height - text_height) / 2.0f;

            C2D_DrawText(&text_obj, C2D_WithColor | C2D_AlignCenter, text_x, text_y, 0.0f,
                         TEXT_SCALE_SMALL, TEXT_SCALE_SMALL, CLR_LIGHT_GRAY);

        } else {
            int track_idx = i - 1;
            u32 bg_color, text_color;
            if (tracks[track_idx].is_muted) {
                bg_color   = CLR_BLACK;
                text_color = CLR_LIGHT_GRAY;
            } else {
                bg_color   = CLR_LIGHT_GRAY;
                text_color = CLR_BLACK;
            }

            C2D_DrawRectangle(0, i * track_height, 0, HOME_TRACKS_WIDTH,
                              track_height - 2, // -2 for spacing
                              bg_color, bg_color, bg_color, bg_color);

            const char *instrument_name = "";
            if (tracks[track_idx].instrument_type == SUB_SYNTH) {
                instrument_name = "Synth";
            } else if (tracks[track_idx].instrument_type == FM_SYNTH) {
                instrument_name = "FM Synth";
            } else if (tracks[track_idx].instrument_type == OPUS_SAMPLER) {
                instrument_name = "Sampler";
            } else if (tracks[track_idx].instrument_type == NOISE_SYNTH) {
                instrument_name = "GB Noize";
            }

            C2D_TextBufClear(text_buf);
            C2D_TextFontParse(&text_obj, font_angular, text_buf, instrument_name);
            C2D_TextOptimize(&text_obj);

            float text_width, text_height;
            C2D_TextGetDimensions(&text_obj, TEXT_SCALE_SMALL, TEXT_SCALE_SMALL, &text_width,
                                  &text_height);

            float text_x = HOME_TRACKS_WIDTH / 2.0f;
            float text_y = (i * track_height) + (track_height - text_height) / 2.0f;

            C2D_DrawText(&text_obj, C2D_WithColor | C2D_AlignCenter, text_x, text_y, 0.0f,
                         TEXT_SCALE_SMALL, TEXT_SCALE_SMALL, text_color);
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

void drawMainView(Track *tracks, int selected_row, int selected_col, ScreenFocus focus) {
    LightLock_Lock(&g_clock_display_lock);
    ClockDisplay clock_display = g_clock_display;
    LightLock_Unlock(&g_clock_display_lock);

    int cur_step       = clock_display.cur_step;
    int steps_per_beat = 4; // Default value, will be updated if tracks[0].sequencer exists

    if (tracks && tracks[0].sequencer) {
        steps_per_beat = tracks[0].sequencer->steps_per_beat;
    }

    drawStepsBar(cur_step, steps_per_beat);
    drawTrackbar(tracks);
    drawTracksSequencers(tracks, cur_step);
    drawSelectionOverlay(selected_row, selected_col, focus == FOCUS_TOP);
}