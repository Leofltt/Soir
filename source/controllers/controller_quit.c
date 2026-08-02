#include "controllers/controller_quit.h"
#include "controllers/controller_project_load.h"
#include "session.h"
#include "project.h"
#include "engine_constants.h"
#include <3ds.h>
#include <stdio.h>
#include <string.h>

void handleInputQuitView(SessionContext *ctx, u32 kDown, bool *should_break_loop) {
    int max_option = 3; // Save, Load, Quit, Cancel
    
    if (kDown & KEY_UP) {
        *ctx->selected_quit_option =
            (*ctx->selected_quit_option > 0) ? *ctx->selected_quit_option - 1 : max_option;
    }
    if (kDown & KEY_DOWN) {
        *ctx->selected_quit_option =
            (*ctx->selected_quit_option < max_option) ? *ctx->selected_quit_option + 1 : 0;
    }
    if (kDown & KEY_B) {
        ctx->session->main_screen_view = VIEW_MAIN;
        *ctx->screen_focus             = *ctx->previous_screen_focus;
    }
    if (kDown & KEY_A) {
        if (*ctx->selected_quit_option == 0) { // Save Submenu
            ctx->session->main_screen_view = VIEW_SAVE_SUBMENU;
            *ctx->selected_quit_option = 0;
        } 
        else if (*ctx->selected_quit_option == 1) { // Load Submenu
            ctx->session->main_screen_view = VIEW_LOAD_SUBMENU;
            *ctx->selected_quit_option = 0;
        }
        else if (*ctx->selected_quit_option == 2) { // Quit
            *should_break_loop = true;
        } 
        else if (*ctx->selected_quit_option == 3) { // Cancel
            ctx->session->main_screen_view = VIEW_MAIN;
            *ctx->screen_focus             = *ctx->previous_screen_focus;
        }
    }
}

void handleInputSaveSubMenu(SessionContext *ctx, u32 kDown) {
    int max_option = 3; // Project, Sequence, Track, Cancel
    
    if (kDown & KEY_UP) {
        *ctx->selected_quit_option =
            (*ctx->selected_quit_option > 0) ? *ctx->selected_quit_option - 1 : max_option;
    }
    if (kDown & KEY_DOWN) {
        *ctx->selected_quit_option =
            (*ctx->selected_quit_option < max_option) ? *ctx->selected_quit_option + 1 : 0;
    }
    if (kDown & KEY_B) {
        ctx->session->main_screen_view = VIEW_QUIT;
        *ctx->selected_quit_option = 0; // Go back highlighting "Save"
    }
    if (kDown & KEY_A) {
        if (*ctx->selected_quit_option == 3) { // Cancel
            ctx->session->main_screen_view = VIEW_QUIT;
            *ctx->selected_quit_option = 0;
            return;
        }

        SwkbdState swkbd;
        char name_buf[MAX_PROJECT_NAME_LEN];
        memset(name_buf, 0, sizeof(name_buf));
        
        swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 2, -1);
        swkbdSetInitialText(&swkbd, "");
        
        if (*ctx->selected_quit_option == 0) {
            swkbdSetHintText(&swkbd, "Enter project name");
        } else if (*ctx->selected_quit_option == 1) {
            swkbdSetHintText(&swkbd, "Enter sequence name");
        } else if (*ctx->selected_quit_option == 2) {
            swkbdSetHintText(&swkbd, "Enter track name");
        }

        SwkbdButton button = swkbdInputText(&swkbd, name_buf, sizeof(name_buf));
        
        if (button == SWKBD_BUTTON_CONFIRM && strlen(name_buf) > 0) {
            if (*ctx->selected_quit_option == 0) { // Project
                saveProject(name_buf, ctx->tracks, ctx->clock, ctx->sample_bank);
            } 
            else if (*ctx->selected_quit_option == 1) { // Sequence
                saveSequenceToFile(name_buf, ctx->tracks, ctx->clock);
            }
            else if (*ctx->selected_quit_option == 2) { // Track
                int active_track = 0;
                if (*ctx->selected_row > 0 && *ctx->selected_row <= N_TRACKS) {
                    active_track = *ctx->selected_row - 1;
                }
                saveTrackToFile(name_buf, active_track, ctx->tracks, true, ctx->clock, ctx->sample_bank);
            }
        }
        
        // Go back to main screen
        ctx->session->main_screen_view = VIEW_MAIN;
        *ctx->screen_focus             = *ctx->previous_screen_focus;
    }
}

void handleInputLoadSubMenu(SessionContext *ctx, u32 kDown) {
    int max_option = 3; // Project, Sequence, Track, Cancel
    
    if (kDown & KEY_UP) {
        *ctx->selected_quit_option =
            (*ctx->selected_quit_option > 0) ? *ctx->selected_quit_option - 1 : max_option;
    }
    if (kDown & KEY_DOWN) {
        *ctx->selected_quit_option =
            (*ctx->selected_quit_option < max_option) ? *ctx->selected_quit_option + 1 : 0;
    }
    if (kDown & KEY_B) {
        ctx->session->main_screen_view = VIEW_QUIT;
        *ctx->selected_quit_option = 1; // Go back highlighting "Load"
    }
    if (kDown & KEY_A) {
        if (*ctx->selected_quit_option == 0) { // Load Project
            enterProjectLoadView(ctx, 0);
        }
        else if (*ctx->selected_quit_option == 1) { // Load Sequence
            enterProjectLoadView(ctx, 1);
        }
        else if (*ctx->selected_quit_option == 2) { // Load Track
            enterProjectLoadView(ctx, 2);
        }
        else if (*ctx->selected_quit_option == 3) { // Cancel
            ctx->session->main_screen_view = VIEW_QUIT;
            *ctx->selected_quit_option = 1;
        }
    }
}