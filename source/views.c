#include "views.h"

#include "engine_constants.h"
#include "ui_constants.h"
#include <stdio.h>

static C2D_Font font_angular;
static C2D_TextBuf text_buf;
static C2D_Text text_obj;

void initViews() {
    font_angular = C2D_FontLoad(FONTPATH_F500ANGULAR);
    text_buf = C2D_TextBufNew(128);
}

void deinitViews() {
    C2D_FontFree(font_angular);
    C2D_TextBufDelete(text_buf);
}

void drawStepsBar(int cur_step) {
    for (int i = 0; i < 16; i++) {
        u32 color = (i == cur_step) ? CLR_RED : CLR_DARK_GRAY;
        C2D_DrawRectangle(HOME_TRACKS_WIDTH + HOME_STEPS_SPACER_W * (i + 1) +
                              HOME_STEPS_HEADER_W * i,
                          0, 0, HOME_STEPS_HEADER_W, HOME_STEPS_HEIGHT, color,
                          color, color, color);
    }
}

static const char* get_status_symbol(ClockStatus status) {
    switch (status) {
        case PLAYING: return "P";
        case STOPPED: return "S";
        case PAUSED: return "||";
        default: return "";
    }
}

void drawTrackbar(Clock *clock) {
    float track_height = SCREEN_HEIGHT / 13;
    for (int i = 0; i < N_TRACKS + 1; i++) {
        if (i == 0) {
            // Draw outline for the header box
            C2D_DrawRectangle(0, 0, 0, HOME_TRACKS_WIDTH, 1, CLR_LIGHT_GRAY, CLR_LIGHT_GRAY, CLR_LIGHT_GRAY, CLR_LIGHT_GRAY);
            C2D_DrawRectangle(0, track_height - 3, 0, HOME_TRACKS_WIDTH, 1, CLR_LIGHT_GRAY, CLR_LIGHT_GRAY, CLR_LIGHT_GRAY, CLR_LIGHT_GRAY);
            C2D_DrawRectangle(0, 0, 0, 1, track_height - 2, CLR_LIGHT_GRAY, CLR_LIGHT_GRAY, CLR_LIGHT_GRAY, CLR_LIGHT_GRAY);
            C2D_DrawRectangle(HOME_TRACKS_WIDTH - 1, 0, 0, 1, track_height - 2, CLR_LIGHT_GRAY, CLR_LIGHT_GRAY, CLR_LIGHT_GRAY, CLR_LIGHT_GRAY);

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

            C2D_DrawText(&text_obj, C2D_WithColor | C2D_AlignCenter, text_x / 2, text_y, 0.0f, 0.4f, 0.4f, CLR_LIGHT_GRAY);

            // Draw bpm status text
            C2D_TextBufClear(text_buf);
            snprintf(buf, sizeof(buf), "%.0f%s", clock->bpm, get_status_symbol(clock->status));
            C2D_TextFontParse(&text_obj, font_angular, text_buf, buf);
            C2D_TextOptimize(&text_obj);

            C2D_TextGetDimensions(&text_obj, 0.4f, 0.4f, &text_width, &text_height);
            text_x = (HOME_TRACKS_WIDTH / 2.0f) + (HOME_TRACKS_WIDTH / 2.0f) / 2.0f;
            text_y = (track_height - text_height) / 2.0f;

            C2D_DrawText(&text_obj, C2D_WithColor | C2D_AlignCenter, text_x, text_y, 0.0f, 0.4f, 0.4f, CLR_LIGHT_GRAY);

        } else {
            C2D_DrawRectangle(0, i * track_height, 0, HOME_TRACKS_WIDTH,
                              track_height - 2, // -2 for spacing
                              CLR_LIGHT_GRAY, CLR_LIGHT_GRAY, CLR_LIGHT_GRAY, CLR_LIGHT_GRAY);
        }
    }
}

void drawTracksSequencers(Track *tracks) {
    float track_height = SCREEN_HEIGHT / 13;
    for (int i = 0; i < N_TRACKS; i++) {
        if (tracks[i].sequencer) {
            for (int j = 0; j < 16; j++) {
                u32 color = tracks[i].sequencer->steps[j].active ? CLR_LIGHT_GRAY : CLR_DARK_GRAY;
                float x = HOME_TRACKS_WIDTH + HOME_STEPS_SPACER_W * (j + 1) +
                          HOME_STEPS_HEADER_W * j;
                float y = (i + 1) * track_height;
                float w = HOME_STEPS_HEADER_W;
                float h = track_height - 2;

                if (j == tracks[i].sequencer->cur_step) {
                    C2D_DrawRectangle(x - 1, y - 1, 0, w + 2, h + 2, CLR_RED, CLR_RED, CLR_RED, CLR_RED);
                }

                C2D_DrawRectangle(x, y, 0, w, h, color, color, color, color);
            }
        }
    }
}

void drawMainView(Track *tracks, Clock *clock) {
    int cur_step = -1; // Default to no active step
    if (tracks && tracks[0].sequencer) {
        cur_step = tracks[0].sequencer->cur_step;
    }
    drawStepsBar(cur_step);
    drawTrackbar(clock);
    drawTracksSequencers(tracks);
}