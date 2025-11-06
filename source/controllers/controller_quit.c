#include "controllers/controller_quit.h"
#include "session.h"

void handleInputQuitView(SessionContext *ctx, u32 kDown, bool *should_break_loop) {
    if (kDown & KEY_UP) {
        *ctx->selectedQuitOption = (*ctx->selectedQuitOption > 0) ? *ctx->selectedQuitOption - 1
                                                                  : 1; // numQuitMenuOptions - 1
    }
    if (kDown & KEY_DOWN) {
        *ctx->selectedQuitOption = (*ctx->selectedQuitOption < 1) ? *ctx->selectedQuitOption + 1
                                                                  : 0; // numQuitMenuOptions - 1
    }
    if (kDown & KEY_B) {
        ctx->session->main_screen_view = VIEW_MAIN;
        *ctx->screen_focus             = *ctx->previous_screen_focus;
    }
    if (kDown & KEY_A) {
        if (*ctx->selectedQuitOption == 0) { // Quit
            *should_break_loop = true;
        } else { // Cancel
            ctx->session->main_screen_view = VIEW_MAIN;
            *ctx->screen_focus             = *ctx->previous_screen_focus;
        }
    }
}