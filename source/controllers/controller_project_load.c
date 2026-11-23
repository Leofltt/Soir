#include "controllers/controller_project_load.h"
#include "project.h"
#include "session.h"
#include "engine_constants.h"
#include <string.h>
#include <stdio.h>

char g_project_names[MAX_PROJECTS][MAX_PROJECT_NAME_LEN];
int  g_project_count = 0;

void enterProjectLoadView(SessionContext *ctx, int mode) {
    *ctx->selected_project_option = 0;
    if (mode == 0) { // Project
        g_project_count = getProjectList("sdmc:/Soir/projects", ".soir", g_project_names);
        ctx->session->main_screen_view = VIEW_PROJECT_LOAD;
    } else if (mode == 1) { // Sequence
        g_project_count = getProjectList("sdmc:/Soir/sequences", ".soirp", g_project_names);
        ctx->session->main_screen_view = VIEW_SEQUENCE_LOAD;
    } else if (mode == 2) { // Track
        g_project_count = getProjectList("sdmc:/Soir/tracks", ".soirt", g_project_names);
        ctx->session->main_screen_view = VIEW_TRACK_LOAD;
    }
}

void handleInputProjectLoad(SessionContext *ctx, u32 kDown, int mode) {
    if (kDown & KEY_B) {
        // Go back to the load submenu
        ctx->session->main_screen_view = VIEW_LOAD_SUBMENU;
        *ctx->selected_quit_option     = mode; // Highlight the selected option
        return;
    }

    if (g_project_count == 0) {
        return;
    }

    if (kDown & KEY_UP) {
        if (*ctx->selected_project_option > 0) {
            (*ctx->selected_project_option)--;
        } else {
            *ctx->selected_project_option = g_project_count - 1;
        }
    }

    if (kDown & KEY_DOWN) {
        if (*ctx->selected_project_option < g_project_count - 1) {
            (*ctx->selected_project_option)++;
        } else {
            *ctx->selected_project_option = 0;
        }
    }

    if (kDown & KEY_A) {
        const char *name = g_project_names[*ctx->selected_project_option];
        if (mode == 2) { // Track
            int active_track = 0;
            if (*ctx->selected_row > 0 && *ctx->selected_row <= N_TRACKS) {
                active_track = *ctx->selected_row - 1;
            }
            loadTrackFromFile(name, &ctx->tracks[active_track]);
        } else if (mode == 1) { // Sequence
            loadSequenceFromFile(name, ctx->tracks, ctx->clock);
        } else if (mode == 0) { // Project
            loadProject(name, ctx->tracks, ctx->clock, ctx->sample_bank);
        }

        // Return to main view
        ctx->session->main_screen_view = VIEW_MAIN;
        *ctx->screen_focus             = *ctx->previous_screen_focus;
    }
}
