#include "views.h"

#include "engine_constants.h"
#include "ui_constants.h"

void drawStepsBar() {
    for (int i = 0; i < 16; i++) {
        C2D_DrawRectangle(
            HOME_TRACKS_WIDTH + HOME_STEPS_SPACER_W * (i + 1) + HOME_STEPS_HEADER_W * i,
            0, 0,
            HOME_STEPS_HEADER_W, HOME_STEPS_HEIGHT,
            CLR_DARK_GRAY, CLR_DARK_GRAY, CLR_DARK_GRAY, CLR_DARK_GRAY);
    }
}

void drawTrackbar() {
    float track_height = SCREEN_HEIGHT / 13; 
    for (int i = 0; i < 13; i++) {
        C2D_DrawRectangle(0, i * track_height, 0,
                         HOME_TRACKS_WIDTH, track_height - 2, // -2 for spacing
                         CLR_LIGHT_GRAY, CLR_LIGHT_GRAY, 
                         CLR_LIGHT_GRAY, CLR_LIGHT_GRAY);
    }
}

void drawMainView() {
    drawStepsBar();
    drawTrackbar();
}