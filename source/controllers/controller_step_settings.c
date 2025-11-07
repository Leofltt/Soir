#include "controllers/controller_step_settings.h"
#include "audio_utils.h"
#include "engine_constants.h"
#include "track.h"
#include "ui/ui.h"
#include "session.h"
#include "track_parameters.h"
#include "sample_bank.h"
#include <string.h>

extern int generateParameterList(Track *track, TrackParameters *params, SampleBank *sample_bank,
                                 ParameterInfo *list_buffer, int max_params);

void initEditingParams(SessionContext *ctx, Track *track, int selected_col) {
    if (selected_col == 0) { // All steps
        memcpy(ctx->editing_step_params, track->default_parameters, sizeof(TrackParameters));
        if (track->instrument_type == SUB_SYNTH) {
            memcpy(ctx->editing_subsynth_params, track->default_parameters->instrument_data,
                   sizeof(SubSynthParameters));
        } else if (track->instrument_type == OPUS_SAMPLER) {
            memcpy(ctx->editing_sampler_params, track->default_parameters->instrument_data,
                   sizeof(OpusSamplerParameters));
        } else if (track->instrument_type == FM_SYNTH) {
            memcpy(ctx->editing_fm_synth_params, track->default_parameters->instrument_data,
                   sizeof(FMSynthParameters));
        } else if (track->instrument_type == NOISE_SYNTH) {
            memcpy(ctx->editing_noise_synth_params, track->default_parameters->instrument_data,
                   sizeof(NoiseSynthParameters));
        }
    } else { // Specific step
        int step_idx = selected_col - 1;
        if (step_idx >= 0) {
            SeqStep *seq_step = &track->sequencer->steps[step_idx];
            memcpy(ctx->editing_step_params, seq_step->data, sizeof(TrackParameters));
            if (track->instrument_type == SUB_SYNTH) {
                memcpy(ctx->editing_subsynth_params, seq_step->data->instrument_data,
                       sizeof(SubSynthParameters));
            } else if (track->instrument_type == OPUS_SAMPLER) {
                memcpy(ctx->editing_sampler_params, seq_step->data->instrument_data,
                       sizeof(OpusSamplerParameters));
            } else if (track->instrument_type == FM_SYNTH) {
                memcpy(ctx->editing_fm_synth_params, seq_step->data->instrument_data,
                       sizeof(FMSynthParameters));
            } else if (track->instrument_type == NOISE_SYNTH) {
                memcpy(ctx->editing_noise_synth_params, seq_step->data->instrument_data,
                       sizeof(NoiseSynthParameters));
            }
        }
    }
}

void handleInputStepSettings(SessionContext *ctx, u32 kDown) {
    int track_idx = *ctx->selected_row - 1;
    if (track_idx < 0 || track_idx >= N_TRACKS)
        return;
    Track *track = &ctx->tracks[track_idx];

    TrackParameters *params_to_show;
    bool             is_all_steps = (*ctx->selected_col == 0);
    if (is_all_steps) {
        params_to_show = track->default_parameters;
    } else {
        int step_idx = *ctx->selected_col - 1;
        if (step_idx >= 0 && track->sequencer && track->sequencer->steps) {
            params_to_show = track->sequencer->steps[step_idx].data;
        } else {
            params_to_show = track->default_parameters; // Fallback
        }
    }

    ParameterInfo param_list[MAX_VIEW_PARAMS];
    int           param_count =
        generateParameterList(track, params_to_show, ctx->sample_bank, param_list, MAX_VIEW_PARAMS);

    int num_left  = 0;
    int num_right = 0;
    for (int i = 0; i < param_count; i++) {
        if (param_list[i].column == 0) {
            num_left++;
        } else {
            num_right++;
        }
    }

    int current_col = (*ctx->selected_step_option < num_left) ? 0 : 1;
    int current_row =
        (current_col == 0) ? *ctx->selected_step_option : *ctx->selected_step_option - num_left;

    if (kDown & KEY_UP) {
        if (current_row > 0)
            current_row--;
    }
    if (kDown & KEY_DOWN) {
        int max_row = (current_col == 0) ? num_left - 1 : num_right - 1;
        if (current_row < max_row)
            current_row++;
    }
    if (kDown & KEY_LEFT) {
        if (current_col > 0 && num_left > 0) {
            current_col = 0;
            if (current_row >= num_left) {
                current_row = num_left - 1;
            }
        }
    }
    if (kDown & KEY_RIGHT) {
        if (current_col < 1 && num_right > 0) {
            current_col = 1;
            if (current_row >= num_right) {
                current_row = num_right - 1;
            }
        }
    }
    if (kDown & KEY_B) {
        *ctx->selected_row = (*ctx->selected_row % N_TRACKS) + 1;
    }

    if (kDown & KEY_X) {
        int n_steps        = track->sequencer->n_beats * track->sequencer->steps_per_beat;
        *ctx->selected_col = (*ctx->selected_col + 1) % (n_steps + 1);
    }

    *ctx->selected_step_option = (current_col == 0) ? current_row : current_row + num_left;

    if (kDown & KEY_A) {
        if (track_idx >= 0) {
            initEditingParams(ctx, track, *ctx->selected_col);
            if (*ctx->selected_step_option >= 0 && *ctx->selected_step_option < param_count) {
                ctx->session->main_screen_view = VIEW_STEP_SETTINGS_EDIT;
                *ctx->screen_focus             = FOCUS_TOP;
                *ctx->selected_adsr_option     = 0; // Reset ADSR selection
            }
        }
    }
}