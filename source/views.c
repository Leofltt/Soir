#include "views.h"

#include "engine_constants.h"
#include "ui_constants.h"

void draw_toolbar() {
    C2D_DrawRectangle(0, 0, 0, HOME_TOOLBAR_WIDTH, SCREEN_HEIGHT, CLR_DARK_GRAY, CLR_DARK_GRAY,
                      CLR_DARK_GRAY, CLR_DARK_GRAY);
}

void draw_trackbar() {
    for (int i = 0; i < N_TRACKS; i++) {
        C2D_DrawRectangle(HOME_TOOLBAR_WIDTH + HOME_TRACKS_SPACER_W * (i + 1) +
                              HOME_TRACKS_HEADER_W * i,
                          0, 0, HOME_TRACKS_HEADER_W, HOME_TRACKS_HEIGHT, CLR_LIGHT_GRAY,
                          CLR_LIGHT_GRAY, CLR_LIGHT_GRAY, CLR_LIGHT_GRAY);
    }
}