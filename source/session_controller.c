#include "session_controller.h"
#include "views.h"
#include "audio_utils.h"
#include <stdbool.h>
#include <string.h>
#include "engine_constants.h"

// Global variables from main.c that are now passed via SessionContext
extern EventQueue    g_event_queue;
extern SampleBank    g_sample_bank;
extern SampleBrowser g_sample_browser;

bool handle_continuous_press(u32 kDown, u32 kHeld, u64 now, u32 key, u64 *timer,
                             const u64 delay_initial, const u64 delay_repeat) {
    if (kDown & key) {
        *timer = now + delay_initial;
        return true;
    }
    if ((kHeld & key) && (now >= *timer)) {
        *timer = now + delay_repeat;
        return true;
    }
    return false;
}

static void initEditingParams(SessionContext *ctx, Track *track, int selected_col) {
    if (selected_col == 0) {
        memcpy(ctx->editing_step_params, track->default_parameters, sizeof(TrackParameters));
        if (track->instrument_type == SUB_SYNTH) {
            memcpy(ctx->editing_subsynth_params, track->default_parameters->instrument_data,
                   sizeof(SubSynthParameters));
            ctx->editing_step_params->instrument_data = ctx->editing_subsynth_params;
        } else if (track->instrument_type == OPUS_SAMPLER) {
            if (!ctx->editing_sampler_params) {
                ctx->editing_sampler_params = linearAlloc(sizeof(OpusSamplerParameters));
            }
            memcpy(ctx->editing_sampler_params, track->default_parameters->instrument_data,
                   sizeof(OpusSamplerParameters));
            ctx->editing_step_params->instrument_data = ctx->editing_sampler_params;
        } else if (track->instrument_type == FM_SYNTH) {
            memcpy(ctx->editing_fm_synth_params, track->default_parameters->instrument_data,
                   sizeof(FMSynthParameters));
            ctx->editing_step_params->instrument_data = ctx->editing_fm_synth_params;
        }
    } else {
        SeqStep *seq_step = &track->sequencer->steps[selected_col - 1];
        memcpy(ctx->editing_step_params, seq_step->data, sizeof(TrackParameters));
        if (track->instrument_type == SUB_SYNTH) {
            memcpy(ctx->editing_subsynth_params, seq_step->data->instrument_data,
                   sizeof(SubSynthParameters));
            ctx->editing_step_params->instrument_data = ctx->editing_subsynth_params;
        } else if (track->instrument_type == OPUS_SAMPLER) {
            if (!ctx->editing_sampler_params) {
                ctx->editing_sampler_params = linearAlloc(sizeof(OpusSamplerParameters));
            }
            memcpy(ctx->editing_sampler_params, seq_step->data->instrument_data,
                   sizeof(OpusSamplerParameters));
            ctx->editing_step_params->instrument_data = ctx->editing_sampler_params;
        } else if (track->instrument_type == FM_SYNTH) {
            memcpy(ctx->editing_fm_synth_params, seq_step->data->instrument_data,
                   sizeof(FMSynthParameters));
            ctx->editing_step_params->instrument_data = ctx->editing_fm_synth_params;
        }
    }
}

void sessionControllerHandleInput(SessionContext *ctx, u32 kDown, u32 kHeld, u64 now,
                                  bool *should_break_loop) {
    if (kDown & KEY_START) {
        *ctx->previous_screen_focus    = *ctx->screen_focus;
        *ctx->screen_focus             = FOCUS_TOP;
        ctx->session->main_screen_view = VIEW_QUIT;
        *ctx->selectedQuitOption       = 0; // Reset selection when opening
        LightLock_Lock(ctx->clock_lock);
        pauseClock(ctx->clock);
        LightLock_Unlock(ctx->clock_lock);
    }

    if (ctx->session->main_screen_view != VIEW_QUIT) {
        if (kDown & KEY_R) {
            if (*ctx->screen_focus == FOCUS_TOP) {
                *ctx->screen_focus = FOCUS_BOTTOM;
            } else {
                *ctx->screen_focus = FOCUS_TOP;
            }
        }
    }

    if (kDown & KEY_L) {
        if (ctx->session->touch_screen_view == VIEW_TOUCH_SETTINGS) {
            ctx->session->touch_screen_view = VIEW_STEP_SETTINGS;
        } else if (ctx->session->touch_screen_view == VIEW_STEP_SETTINGS) {
            ctx->session->touch_screen_view = VIEW_TOUCH_SETTINGS;
        }
    }

    if (*ctx->screen_focus == FOCUS_TOP) {
        switch (ctx->session->main_screen_view) {
        case VIEW_MAIN: {
            if (handle_continuous_press(kDown, kHeld, now, KEY_UP, ctx->up_timer,
                                        ctx->HOLD_DELAY_INITIAL, ctx->HOLD_DELAY_REPEAT)) {
                *ctx->selected_row = (*ctx->selected_row > 0) ? *ctx->selected_row - 1 : N_TRACKS;
            }

            if (handle_continuous_press(kDown, kHeld, now, KEY_DOWN, ctx->down_timer,
                                        ctx->HOLD_DELAY_INITIAL, ctx->HOLD_DELAY_REPEAT)) {
                *ctx->selected_row = (*ctx->selected_row < N_TRACKS) ? *ctx->selected_row + 1 : 0;
            }

            if (handle_continuous_press(kDown, kHeld, now, KEY_LEFT, ctx->left_timer,
                                        ctx->HOLD_DELAY_INITIAL, ctx->HOLD_DELAY_REPEAT)) {
                *ctx->selected_col = (*ctx->selected_col > 0) ? *ctx->selected_col - 1 : 16;
            }

            if (handle_continuous_press(kDown, kHeld, now, KEY_RIGHT, ctx->right_timer,
                                        ctx->HOLD_DELAY_INITIAL, ctx->HOLD_DELAY_REPEAT)) {
                *ctx->selected_col = (*ctx->selected_col < 16) ? *ctx->selected_col + 1 : 0;
            }

            if (kDown & KEY_Y) {
                if (*ctx->selected_row == 0 && *ctx->selected_col == 0) {
                    LightLock_Lock(ctx->clock_lock);
                    if (ctx->clock->status == PLAYING) {
                        pauseClock(ctx->clock);
                    } else if (ctx->clock->status == PAUSED) {
                        resumeClock(ctx->clock);
                    }
                    LightLock_Unlock(ctx->clock_lock);
                } else if (*ctx->selected_row > 0 && *ctx->selected_col > 0) {
                    ctx->session->touch_screen_view = VIEW_STEP_SETTINGS;
                    *ctx->screen_focus              = FOCUS_BOTTOM;
                }
            }

            if (kDown & KEY_A) {
                if (*ctx->selected_row == 0 && *ctx->selected_col == 0) {
                    LightLock_Lock(ctx->clock_lock);
                    stopClock(ctx->clock);
                    LightLock_Unlock(ctx->clock_lock);
                    ctx->session->main_screen_view = VIEW_SETTINGS;
                    *ctx->selected_settings_option = 0;
                } else if (*ctx->selected_row > 0 && *ctx->selected_col == 0) {
                    int track_index = *ctx->selected_row - 1;
                    if (track_index < N_TRACKS) {
                        LightLock_Lock(ctx->tracks_lock);
                        ctx->tracks[track_index].is_muted = !ctx->tracks[track_index].is_muted;
                        for (int i = 0; i < ctx->tracks[track_index].sequencer->n_beats *
                                                ctx->tracks[track_index].sequencer->steps_per_beat;
                             i++) {
                            ctx->tracks[track_index].sequencer->steps[i].data->is_muted =
                                ctx->tracks[track_index].is_muted;
                        }
                        LightLock_Unlock(ctx->tracks_lock);
                    }
                } else if (*ctx->selected_col > 0) {
                    int step_index = *ctx->selected_col - 1;
                    LightLock_Lock(ctx->tracks_lock);
                    if (*ctx->selected_row == 0) { // Header row
                        for (int i = 0; i < N_TRACKS; i++) {
                            if (ctx->tracks[i].sequencer && ctx->tracks[i].sequencer->steps) {
                                ctx->tracks[i].sequencer->steps[step_index].active =
                                    !ctx->tracks[i].sequencer->steps[step_index].active;
                            }
                        }
                    } else { // Track row
                        int track_index = *ctx->selected_row - 1;
                        if (track_index < N_TRACKS && ctx->tracks[track_index].sequencer &&
                            ctx->tracks[track_index].sequencer->steps) {
                            ctx->tracks[track_index].sequencer->steps[step_index].active =
                                !ctx->tracks[track_index].sequencer->steps[step_index].active;
                        }
                    }
                    LightLock_Unlock(ctx->tracks_lock);
                }
            }

            if (kDown & KEY_X) {
                if (*ctx->selected_row == 0 && *ctx->selected_col == 0) {
                    LightLock_Lock(ctx->clock_lock);
                    if (ctx->clock->status == PLAYING || ctx->clock->status == PAUSED) {
                        stopClock(ctx->clock);
                        LightLock_Lock(ctx->tracks_lock);
                        for (int i = 0; i < N_TRACKS; i++) {
                            if (ctx->tracks[i].sequencer) {
                                ctx->tracks[i].sequencer->cur_step = 0;
                            }
                        }
                        LightLock_Unlock(ctx->tracks_lock);
                    } else {
                        startClock(ctx->clock);
                    }
                    LightLock_Unlock(ctx->clock_lock);
                }
            }
        } break;
        case VIEW_SETTINGS:
            if (kDown & KEY_UP) {
                *ctx->selected_settings_option =
                    (*ctx->selected_settings_option > 0) ? *ctx->selected_settings_option - 1 : 2;
            }
            if (kDown & KEY_DOWN) {
                *ctx->selected_settings_option =
                    (*ctx->selected_settings_option < 2) ? *ctx->selected_settings_option + 1 : 0;
            }
            if (kDown & KEY_B) {
                ctx->session->main_screen_view = VIEW_MAIN;
            }
            if (kDown & KEY_A && *ctx->selected_settings_option == 2) {
                ctx->session->main_screen_view = VIEW_MAIN;
            }
            if (handle_continuous_press(kDown, kHeld, now, KEY_LEFT, ctx->left_timer,
                                        ctx->HOLD_DELAY_INITIAL, ctx->HOLD_DELAY_REPEAT)) {
                if (*ctx->selected_settings_option == 0) { // BPM
                    setBpm(ctx->clock, ctx->clock->bpm - 1);
                } else if (*ctx->selected_touch_clock_option == 1) { // Beats per bar
                    setBeatsPerBar(ctx->clock, ctx->clock->barBeats->beats_per_bar - 1);
                }
            }
            if (handle_continuous_press(kDown, kHeld, now, KEY_RIGHT, ctx->right_timer,
                                        ctx->HOLD_DELAY_INITIAL, ctx->HOLD_DELAY_REPEAT)) {
                if (*ctx->selected_settings_option == 0) { // BPM
                    setBpm(ctx->clock, ctx->clock->bpm + 1);
                } else if (*ctx->selected_settings_option == 1) { // Beats per bar
                    setBeatsPerBar(ctx->clock, ctx->clock->barBeats->beats_per_bar + 1);
                }
            }
            break;
        case VIEW_QUIT:
            if (kDown & KEY_UP) {
                *ctx->selectedQuitOption = (*ctx->selectedQuitOption > 0)
                                               ? *ctx->selectedQuitOption - 1
                                               : 1; // numQuitMenuOptions - 1
            }
            if (kDown & KEY_DOWN) {
                *ctx->selectedQuitOption = (*ctx->selectedQuitOption < 1)
                                               ? *ctx->selectedQuitOption + 1
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
            break;
        case VIEW_ABOUT:
            break;
        case VIEW_STEP_SETTINGS_EDIT: {
            int track_idx = *ctx->selected_row - 1;
            if (track_idx < 0)
                break;
            Track *track = &ctx->tracks[track_idx];

            if (kDown & KEY_B) { // Cancel
                ctx->session->main_screen_view = VIEW_MAIN;
                *ctx->screen_focus             = FOCUS_BOTTOM;
            } else if (kDown & KEY_A) {
                if (*ctx->selected_col == 0) {
                    // Apply to all steps
                    for (int i = 0;
                         i < track->sequencer->n_beats * track->sequencer->steps_per_beat; i++) {
                        SeqStep *seq_step                 = &track->sequencer->steps[i];
                        void    *original_instrument_data = seq_step->data->instrument_data;
                        memcpy(seq_step->data, ctx->editing_step_params, sizeof(TrackParameters));
                        seq_step->data->instrument_data = original_instrument_data;

                        if (track->instrument_type == SUB_SYNTH) {
                            memcpy(seq_step->data->instrument_data, ctx->editing_subsynth_params,
                                   sizeof(SubSynthParameters));
                        } else if (track->instrument_type == OPUS_SAMPLER) {
                            memcpy(seq_step->data->instrument_data, ctx->editing_sampler_params,
                                   sizeof(OpusSamplerParameters));
                        } else if (track->instrument_type == FM_SYNTH) {
                            memcpy(seq_step->data->instrument_data, ctx->editing_fm_synth_params,
                                   sizeof(FMSynthParameters));
                        }

                        Event event = { .type            = UPDATE_STEP,
                                        .track_id        = track_idx,
                                        .base_params     = *seq_step->data,
                                        .instrument_type = track->instrument_type };

                        if (track->instrument_type == SUB_SYNTH) {
                            memcpy(&event.instrument_specific_params.subsynth_params,
                                   seq_step->data->instrument_data, sizeof(SubSynthParameters));
                        } else if (track->instrument_type == OPUS_SAMPLER) {
                            memcpy(&event.instrument_specific_params.sampler_params,
                                   seq_step->data->instrument_data, sizeof(OpusSamplerParameters));
                        } else if (track->instrument_type == FM_SYNTH) {
                            memcpy(&event.instrument_specific_params.fm_synth_params,
                                   seq_step->data->instrument_data, sizeof(FMSynthParameters));
                        }
                        eventQueuePush(ctx->event_queue, event);
                    }

                    // Update default parameters for the track
                    void *original_instrument_data = track->default_parameters->instrument_data;
                    memcpy(track->default_parameters, ctx->editing_step_params,
                           sizeof(TrackParameters));
                    track->default_parameters->instrument_data = original_instrument_data;

                    if (track->instrument_type == SUB_SYNTH) {
                        memcpy(track->default_parameters->instrument_data,
                               ctx->editing_subsynth_params, sizeof(SubSynthParameters));
                    } else if (track->instrument_type == OPUS_SAMPLER) {
                        memcpy(track->default_parameters->instrument_data,
                               ctx->editing_sampler_params, sizeof(OpusSamplerParameters));
                    } else if (track->instrument_type == FM_SYNTH) {
                        memcpy(track->default_parameters->instrument_data,
                               ctx->editing_fm_synth_params, sizeof(FMSynthParameters));
                    }
                } else {
                    int step_idx = *ctx->selected_col - 1;
                    if (step_idx >= 0) {
                        SeqStep *seq_step = &track->sequencer->steps[step_idx];

                        void *original_instrument_data = seq_step->data->instrument_data;
                        memcpy(seq_step->data, ctx->editing_step_params, sizeof(TrackParameters));
                        seq_step->data->instrument_data = original_instrument_data;

                        if (track->instrument_type == SUB_SYNTH) {
                            memcpy(seq_step->data->instrument_data, ctx->editing_subsynth_params,
                                   sizeof(SubSynthParameters));
                        } else if (track->instrument_type == OPUS_SAMPLER) {
                            memcpy(seq_step->data->instrument_data, ctx->editing_sampler_params,
                                   sizeof(OpusSamplerParameters));
                        } else if (track->instrument_type == FM_SYNTH) {
                            memcpy(seq_step->data->instrument_data, ctx->editing_fm_synth_params,
                                   sizeof(FMSynthParameters));
                        }
                    }
                }
                track->filter.update_params    = true;
                ctx->session->main_screen_view = VIEW_MAIN;
                *ctx->screen_focus             = FOCUS_BOTTOM;
            } else {
                ParameterInfo param_list[MAX_VIEW_PARAMS];
                int           param_count = generateParameterList(
                    track, ctx->editing_step_params, ctx->sample_bank, param_list, MAX_VIEW_PARAMS);

                ParameterInfo *param_to_edit = NULL;
                for (int i = 0; i < param_count; i++) {
                    if (param_list[i].unique_id == *ctx->selected_step_option) {
                        param_to_edit = &param_list[i];
                        break;
                    }
                }

                if (param_to_edit) {
                    switch (param_to_edit->type) {
                    case PARAM_TYPE_FLOAT_0_1: {
                        if (strcmp(param_to_edit->label, "Mod Depth") == 0) {
                            FMSynthParameters *fm_params = ctx->editing_fm_synth_params;
                            if (handle_continuous_press(kDown, kHeld, now, KEY_UP, ctx->up_timer,
                                                        ctx->HOLD_DELAY_INITIAL,
                                                        ctx->HOLD_DELAY_REPEAT))
                                fm_params->mod_depth += 1.0f;
                            if (handle_continuous_press(kDown, kHeld, now, KEY_DOWN,
                                                        ctx->down_timer, ctx->HOLD_DELAY_INITIAL,
                                                        ctx->HOLD_DELAY_REPEAT))
                                fm_params->mod_depth -= 1.0f;
                            fm_params->mod_depth = clamp(fm_params->mod_depth, 0.0f, 1000.0f);
                        } else {
                            float *value_ptr = NULL;
                            if (strcmp(param_to_edit->label, "Volume") == 0)
                                value_ptr = &ctx->editing_step_params->volume;
                            else if (strcmp(param_to_edit->label, "Mod Index") == 0)
                                value_ptr = &((FMSynthParameters *) ctx->editing_fm_synth_params)
                                                 ->mod_index;
                            else if (strcmp(param_to_edit->label, "Start Pos") == 0) {
                                OpusSamplerParameters *sampler_params = ctx->editing_sampler_params;
                                Sample *sample = SampleBankGetSample(ctx->sample_bank,
                                                                     sampler_params->sample_index);
                                if (sample && sample->pcm_length > 0) {
                                    float pos =
                                        (float) sampler_params->start_position / sample->pcm_length;
                                    if (handle_continuous_press(
                                            kDown, kHeld, now, KEY_UP, ctx->up_timer,
                                            ctx->HOLD_DELAY_INITIAL, ctx->HOLD_DELAY_REPEAT))
                                        pos += 0.01f;
                                    if (handle_continuous_press(
                                            kDown, kHeld, now, KEY_DOWN, ctx->down_timer,
                                            ctx->HOLD_DELAY_INITIAL, ctx->HOLD_DELAY_REPEAT))
                                        pos -= 0.01f;
                                    pos                            = clamp(pos, 0.0f, 1.0f);
                                    sampler_params->start_position = pos * sample->pcm_length;
                                }
                            }

                            if (value_ptr) {
                                if (handle_continuous_press(
                                        kDown, kHeld, now, KEY_DOWN, ctx->down_timer,
                                        ctx->HOLD_DELAY_INITIAL, ctx->HOLD_DELAY_REPEAT))
                                    *value_ptr -= 0.1f;
                                if (handle_continuous_press(kDown, kHeld, now, KEY_UP,
                                                            ctx->up_timer, ctx->HOLD_DELAY_INITIAL,
                                                            ctx->HOLD_DELAY_REPEAT))
                                    *value_ptr += 0.1f;
                                *value_ptr = clamp(*value_ptr, 0.0f, 1.0f);
                            }
                        }
                        break;
                    }
                    case PARAM_TYPE_FLOAT_N1_1: {
                        if (strcmp(param_to_edit->label, "Pan") == 0) {
                            if (handle_continuous_press(kDown, kHeld, now, KEY_DOWN,
                                                        ctx->down_timer, ctx->HOLD_DELAY_INITIAL,
                                                        ctx->HOLD_DELAY_REPEAT))
                                ctx->editing_step_params->pan -= 0.1f;
                            if (handle_continuous_press(kDown, kHeld, now, KEY_UP, ctx->up_timer,
                                                        ctx->HOLD_DELAY_INITIAL,
                                                        ctx->HOLD_DELAY_REPEAT))
                                ctx->editing_step_params->pan += 0.1f;
                            ctx->editing_step_params->pan =
                                clamp(ctx->editing_step_params->pan, -1.0f, 1.0f);
                        }
                        break;
                    }
                    case PARAM_TYPE_HZ: {
                        if (strcmp(param_to_edit->label, "Filter Cf") == 0) {
                            if (handle_continuous_press(kDown, kHeld, now, KEY_UP, ctx->up_timer,
                                                        ctx->HOLD_DELAY_INITIAL,
                                                        ctx->HOLD_DELAY_REPEAT))
                                ctx->editing_step_params->ndsp_filter_cutoff += 100;
                            if (handle_continuous_press(kDown, kHeld, now, KEY_DOWN,
                                                        ctx->down_timer, ctx->HOLD_DELAY_INITIAL,
                                                        ctx->HOLD_DELAY_REPEAT))
                                ctx->editing_step_params->ndsp_filter_cutoff -= 100;
                            ctx->editing_step_params->ndsp_filter_cutoff =
                                clamp(ctx->editing_step_params->ndsp_filter_cutoff, 0, 20000);
                        }
                        break;
                    }
                    case PARAM_TYPE_MIDI_NOTE: {
                        float *freq_ptr = NULL;
                        if (track->instrument_type == SUB_SYNTH)
                            freq_ptr =
                                &((SubSynthParameters *) ctx->editing_subsynth_params)->osc_freq;
                        else if (track->instrument_type == FM_SYNTH)
                            freq_ptr =
                                &((FMSynthParameters *) ctx->editing_fm_synth_params)->carrier_freq;

                        if (freq_ptr) {
                            int midi_note = hertzToMidi(*freq_ptr);
                            if (handle_continuous_press(kDown, kHeld, now, KEY_UP, ctx->up_timer,
                                                        ctx->HOLD_DELAY_INITIAL,
                                                        ctx->HOLD_DELAY_REPEAT))
                                midi_note++;
                            if (handle_continuous_press(kDown, kHeld, now, KEY_DOWN,
                                                        ctx->down_timer, ctx->HOLD_DELAY_INITIAL,
                                                        ctx->HOLD_DELAY_REPEAT))
                                midi_note--;
                            midi_note = clamp(midi_note, 0, 127);
                            *freq_ptr = midiToHertz(midi_note);
                        }
                        break;
                    }
                    case PARAM_TYPE_MOD_RATIO: {
                        if (track->instrument_type == FM_SYNTH) {
                            FMSynthParameters *fm_params = ctx->editing_fm_synth_params;
                            if (handle_continuous_press(kDown, kHeld, now, KEY_UP, ctx->up_timer,
                                                        ctx->HOLD_DELAY_INITIAL,
                                                        ctx->HOLD_DELAY_REPEAT))
                                fm_params->mod_freq_ratio += 0.1f;
                            if (handle_continuous_press(kDown, kHeld, now, KEY_DOWN,
                                                        ctx->down_timer, ctx->HOLD_DELAY_INITIAL,
                                                        ctx->HOLD_DELAY_REPEAT))
                                fm_params->mod_freq_ratio -= 0.1f;
                            if (fm_params->mod_freq_ratio < 0.0f)
                                fm_params->mod_freq_ratio = 0.0f;
                        }
                        break;
                    }
                    case PARAM_TYPE_FILTER_TYPE: {
                        if (kDown & KEY_UP)
                            ctx->editing_step_params->ndsp_filter_type =
                                (ctx->editing_step_params->ndsp_filter_type + 1) % 6;
                        if (kDown & KEY_DOWN)
                            ctx->editing_step_params->ndsp_filter_type =
                                (ctx->editing_step_params->ndsp_filter_type - 1 + 6) % 6;
                        break;
                    }
                    case PARAM_TYPE_WAVEFORM: {
                        if (track->instrument_type == SUB_SYNTH) {
                            SubSynthParameters *synth_params = ctx->editing_subsynth_params;
                            if (kDown & KEY_UP)
                                synth_params->osc_waveform =
                                    (synth_params->osc_waveform + 1) % WAVEFORM_COUNT;
                            if (kDown & KEY_DOWN)
                                synth_params->osc_waveform =
                                    (synth_params->osc_waveform - 1 + WAVEFORM_COUNT) %
                                    WAVEFORM_COUNT;
                        }
                        break;
                    }
                    case PARAM_TYPE_PLAYBACK_MODE: {
                        if (track->instrument_type == OPUS_SAMPLER) {
                            OpusSamplerParameters *sampler_params = ctx->editing_sampler_params;
                            if (kDown & KEY_UP || kDown & KEY_DOWN)
                                sampler_params->playback_mode =
                                    (sampler_params->playback_mode == ONE_SHOT) ? LOOP : ONE_SHOT;
                        }
                        break;
                    }
                    case PARAM_TYPE_SAMPLE_INDEX: {
                        if (track->instrument_type == OPUS_SAMPLER) {
                            OpusSamplerParameters *sampler_params = ctx->editing_sampler_params;
                            int count = SampleBankGetLoadedSampleCount(ctx->sample_bank);
                            if (count > 0) {
                                if (kDown & KEY_UP)
                                    sampler_params->sample_index =
                                        (sampler_params->sample_index + 1) % count;
                                if (kDown & KEY_DOWN)
                                    sampler_params->sample_index =
                                        (sampler_params->sample_index - 1 + count) % count;
                            }
                        }
                        break;
                    }
                    case PARAM_TYPE_INT: {
                        int *value_ptr = NULL;
                        if (track->instrument_type == SUB_SYNTH)
                            value_ptr =
                                &((SubSynthParameters *) ctx->editing_subsynth_params)->env_dur;
                        else if (track->instrument_type == OPUS_SAMPLER)
                            value_ptr =
                                &((OpusSamplerParameters *) ctx->editing_sampler_params)->env_dur;
                        else if (track->instrument_type == FM_SYNTH)
                            value_ptr =
                                &((FMSynthParameters *) ctx->editing_fm_synth_params)->env_dur;

                        if (value_ptr) {
                            if (handle_continuous_press(kDown, kHeld, now, KEY_DOWN,
                                                        ctx->down_timer, ctx->HOLD_DELAY_INITIAL,
                                                        ctx->HOLD_DELAY_REPEAT))
                                *value_ptr -= 10;
                            if (handle_continuous_press(kDown, kHeld, now, KEY_UP, ctx->up_timer,
                                                        ctx->HOLD_DELAY_INITIAL,
                                                        ctx->HOLD_DELAY_REPEAT))
                                *value_ptr += 10;
                            if (*value_ptr < 0)
                                *value_ptr = 0;
                        }
                        break;
                    }
                    case PARAM_TYPE_ENVELOPE_BUTTON: {
                        if (kDown & KEY_LEFT)
                            *ctx->selected_adsr_option = (*ctx->selected_adsr_option > 0)
                                                             ? *ctx->selected_adsr_option - 1
                                                             : 3;
                        if (kDown & KEY_RIGHT)
                            *ctx->selected_adsr_option = (*ctx->selected_adsr_option < 3)
                                                             ? *ctx->selected_adsr_option + 1
                                                             : 0;

                        if (track->instrument_type == SUB_SYNTH) {
                            SubSynthParameters *synth_params = ctx->editing_subsynth_params;

                            if (*ctx->selected_adsr_option == 0) { // Attack

                                if (handle_continuous_press(kDown, kHeld, now, KEY_UP,
                                                            ctx->up_timer, ctx->HOLD_DELAY_INITIAL,
                                                            ctx->HOLD_DELAY_REPEAT))
                                    synth_params->env_atk += 10;

                                if (handle_continuous_press(
                                        kDown, kHeld, now, KEY_DOWN, ctx->down_timer,
                                        ctx->HOLD_DELAY_INITIAL, ctx->HOLD_DELAY_REPEAT))
                                    synth_params->env_atk -= 10;

                                if (synth_params->env_atk < 0)
                                    synth_params->env_atk = 0;

                            } else if (*ctx->selected_adsr_option == 1) { // Decay

                                if (handle_continuous_press(kDown, kHeld, now, KEY_UP,
                                                            ctx->up_timer, ctx->HOLD_DELAY_INITIAL,
                                                            ctx->HOLD_DELAY_REPEAT))
                                    synth_params->env_dec += 10;

                                if (handle_continuous_press(
                                        kDown, kHeld, now, KEY_DOWN, ctx->down_timer,
                                        ctx->HOLD_DELAY_INITIAL, ctx->HOLD_DELAY_REPEAT))
                                    synth_params->env_dec -= 10;

                                if (synth_params->env_dec < 0)
                                    synth_params->env_dec = 0;

                            } else if (*ctx->selected_adsr_option == 2) { // Sustain

                                if (handle_continuous_press(kDown, kHeld, now, KEY_UP,
                                                            ctx->up_timer, ctx->HOLD_DELAY_INITIAL,
                                                            ctx->HOLD_DELAY_REPEAT))
                                    synth_params->env_sus_level += 0.1f;

                                if (handle_continuous_press(
                                        kDown, kHeld, now, KEY_DOWN, ctx->down_timer,
                                        ctx->HOLD_DELAY_INITIAL, ctx->HOLD_DELAY_REPEAT))
                                    synth_params->env_sus_level -= 0.1f;

                                synth_params->env_sus_level =
                                    clamp(synth_params->env_sus_level, 0.0f, 1.0f);

                            } else if (*ctx->selected_adsr_option == 3) { // Release

                                if (handle_continuous_press(kDown, kHeld, now, KEY_UP,
                                                            ctx->up_timer, ctx->HOLD_DELAY_INITIAL,
                                                            ctx->HOLD_DELAY_REPEAT))
                                    synth_params->env_rel += 10;

                                if (handle_continuous_press(
                                        kDown, kHeld, now, KEY_DOWN, ctx->down_timer,
                                        ctx->HOLD_DELAY_INITIAL, ctx->HOLD_DELAY_REPEAT))
                                    synth_params->env_rel -= 10;

                                if (synth_params->env_rel < 0)
                                    synth_params->env_rel = 0;
                            }

                        } else if (track->instrument_type == FM_SYNTH) {
                            FMSynthParameters *fm_synth_params = ctx->editing_fm_synth_params;
                            if (strcmp(param_to_edit->label, "Car Env") == 0) {
                                if (*ctx->selected_adsr_option == 0) { // Attack
                                    if (handle_continuous_press(
                                            kDown, kHeld, now, KEY_UP, ctx->up_timer,
                                            ctx->HOLD_DELAY_INITIAL, ctx->HOLD_DELAY_REPEAT))
                                        fm_synth_params->carrier_env_atk += 10;
                                    if (handle_continuous_press(
                                            kDown, kHeld, now, KEY_DOWN, ctx->down_timer,
                                            ctx->HOLD_DELAY_INITIAL, ctx->HOLD_DELAY_REPEAT))
                                        fm_synth_params->carrier_env_atk -= 10;
                                    if (fm_synth_params->carrier_env_atk < 0)
                                        fm_synth_params->carrier_env_atk = 0;
                                } else if (*ctx->selected_adsr_option == 1) { // Decay
                                    if (handle_continuous_press(
                                            kDown, kHeld, now, KEY_UP, ctx->up_timer,
                                            ctx->HOLD_DELAY_INITIAL, ctx->HOLD_DELAY_REPEAT))
                                        fm_synth_params->carrier_env_dec += 10;
                                    if (handle_continuous_press(
                                            kDown, kHeld, now, KEY_DOWN, ctx->down_timer,
                                            ctx->HOLD_DELAY_INITIAL, ctx->HOLD_DELAY_REPEAT))
                                        fm_synth_params->carrier_env_dec -= 10;
                                    if (fm_synth_params->carrier_env_dec < 0)
                                        fm_synth_params->carrier_env_dec = 0;
                                } else if (*ctx->selected_adsr_option == 2) { // Sustain
                                    if (handle_continuous_press(
                                            kDown, kHeld, now, KEY_UP, ctx->up_timer,
                                            ctx->HOLD_DELAY_INITIAL, ctx->HOLD_DELAY_REPEAT))
                                        fm_synth_params->carrier_env_sus_level += 0.1f;
                                    if (handle_continuous_press(
                                            kDown, kHeld, now, KEY_DOWN, ctx->down_timer,
                                            ctx->HOLD_DELAY_INITIAL, ctx->HOLD_DELAY_REPEAT))
                                        fm_synth_params->carrier_env_sus_level -= 0.1f;
                                    fm_synth_params->carrier_env_sus_level =
                                        clamp(fm_synth_params->carrier_env_sus_level, 0.0f, 1.0f);
                                } else if (*ctx->selected_adsr_option == 3) { // Release
                                    if (handle_continuous_press(
                                            kDown, kHeld, now, KEY_UP, ctx->up_timer,
                                            ctx->HOLD_DELAY_INITIAL, ctx->HOLD_DELAY_REPEAT))
                                        fm_synth_params->carrier_env_rel += 10;
                                    if (handle_continuous_press(
                                            kDown, kHeld, now, KEY_DOWN, ctx->down_timer,
                                            ctx->HOLD_DELAY_INITIAL, ctx->HOLD_DELAY_REPEAT))
                                        fm_synth_params->carrier_env_rel -= 10;
                                    if (fm_synth_params->carrier_env_rel < 0)
                                        fm_synth_params->carrier_env_rel = 0;
                                }
                            } else {                                   // Mod Env
                                if (*ctx->selected_adsr_option == 0) { // Attack
                                    if (handle_continuous_press(
                                            kDown, kHeld, now, KEY_UP, ctx->up_timer,
                                            ctx->HOLD_DELAY_INITIAL, ctx->HOLD_DELAY_REPEAT))
                                        fm_synth_params->mod_env_atk += 10;
                                    if (handle_continuous_press(
                                            kDown, kHeld, now, KEY_DOWN, ctx->down_timer,
                                            ctx->HOLD_DELAY_INITIAL, ctx->HOLD_DELAY_REPEAT))
                                        fm_synth_params->mod_env_atk -= 10;
                                    if (fm_synth_params->mod_env_atk < 0)
                                        fm_synth_params->mod_env_atk = 0;
                                } else if (*ctx->selected_adsr_option == 1) { // Decay
                                    if (handle_continuous_press(
                                            kDown, kHeld, now, KEY_UP, ctx->up_timer,
                                            ctx->HOLD_DELAY_INITIAL, ctx->HOLD_DELAY_REPEAT))
                                        fm_synth_params->mod_env_dec += 10;
                                    if (handle_continuous_press(
                                            kDown, kHeld, now, KEY_DOWN, ctx->down_timer,
                                            ctx->HOLD_DELAY_INITIAL, ctx->HOLD_DELAY_REPEAT))
                                        fm_synth_params->mod_env_dec -= 10;
                                    if (fm_synth_params->mod_env_dec < 0)
                                        fm_synth_params->mod_env_dec = 0;
                                } else if (*ctx->selected_adsr_option == 2) { // Sustain
                                    if (handle_continuous_press(
                                            kDown, kHeld, now, KEY_UP, ctx->up_timer,
                                            ctx->HOLD_DELAY_INITIAL, ctx->HOLD_DELAY_REPEAT))
                                        fm_synth_params->mod_env_sus_level += 0.1f;
                                    if (handle_continuous_press(
                                            kDown, kHeld, now, KEY_DOWN, ctx->down_timer,
                                            ctx->HOLD_DELAY_INITIAL, ctx->HOLD_DELAY_REPEAT))
                                        fm_synth_params->mod_env_sus_level -= 0.1f;
                                    fm_synth_params->mod_env_sus_level =
                                        clamp(fm_synth_params->mod_env_sus_level, 0.0f, 1.0f);
                                } else if (*ctx->selected_adsr_option == 3) { // Release
                                    if (handle_continuous_press(
                                            kDown, kHeld, now, KEY_UP, ctx->up_timer,
                                            ctx->HOLD_DELAY_INITIAL, ctx->HOLD_DELAY_REPEAT))
                                        fm_synth_params->mod_env_rel += 10;
                                    if (handle_continuous_press(
                                            kDown, kHeld, now, KEY_DOWN, ctx->down_timer,
                                            ctx->HOLD_DELAY_INITIAL, ctx->HOLD_DELAY_REPEAT))
                                        fm_synth_params->mod_env_rel -= 10;
                                    if (fm_synth_params->mod_env_rel < 0)
                                        fm_synth_params->mod_env_rel = 0;
                                }
                            }
                        } else if (track->instrument_type == OPUS_SAMPLER) {
                            OpusSamplerParameters *sampler_params = ctx->editing_sampler_params;
                            if (*ctx->selected_adsr_option == 0) { // Attack
                                if (handle_continuous_press(kDown, kHeld, now, KEY_UP,
                                                            ctx->up_timer, ctx->HOLD_DELAY_INITIAL,
                                                            ctx->HOLD_DELAY_REPEAT))
                                    sampler_params->env_atk += 10;
                                if (handle_continuous_press(
                                        kDown, kHeld, now, KEY_DOWN, ctx->down_timer,
                                        ctx->HOLD_DELAY_INITIAL, ctx->HOLD_DELAY_REPEAT))
                                    sampler_params->env_atk -= 10;
                                if (sampler_params->env_atk < 0)
                                    sampler_params->env_atk = 0;
                            } else if (*ctx->selected_adsr_option == 1) { // Decay
                                if (handle_continuous_press(kDown, kHeld, now, KEY_UP,
                                                            ctx->up_timer, ctx->HOLD_DELAY_INITIAL,
                                                            ctx->HOLD_DELAY_REPEAT))
                                    sampler_params->env_dec += 10;
                                if (handle_continuous_press(
                                        kDown, kHeld, now, KEY_DOWN, ctx->down_timer,
                                        ctx->HOLD_DELAY_INITIAL, ctx->HOLD_DELAY_REPEAT))
                                    sampler_params->env_dec -= 10;
                                if (sampler_params->env_dec < 0)
                                    sampler_params->env_dec = 0;
                            } else if (*ctx->selected_adsr_option == 2) { // Sustain
                                if (handle_continuous_press(kDown, kHeld, now, KEY_UP,
                                                            ctx->up_timer, ctx->HOLD_DELAY_INITIAL,
                                                            ctx->HOLD_DELAY_REPEAT))
                                    sampler_params->env_sus_level += 0.1f;
                                if (handle_continuous_press(
                                        kDown, kHeld, now, KEY_DOWN, ctx->down_timer,
                                        ctx->HOLD_DELAY_INITIAL, ctx->HOLD_DELAY_REPEAT))
                                    sampler_params->env_sus_level -= 0.1f;
                                sampler_params->env_sus_level =
                                    clamp(sampler_params->env_sus_level, 0.0f, 1.0f);
                            } else if (*ctx->selected_adsr_option == 3) { // Release
                                if (handle_continuous_press(kDown, kHeld, now, KEY_UP,
                                                            ctx->up_timer, ctx->HOLD_DELAY_INITIAL,
                                                            ctx->HOLD_DELAY_REPEAT))
                                    sampler_params->env_rel += 10;
                                if (handle_continuous_press(
                                        kDown, kHeld, now, KEY_DOWN, ctx->down_timer,
                                        ctx->HOLD_DELAY_INITIAL, ctx->HOLD_DELAY_REPEAT))
                                    sampler_params->env_rel -= 10;
                                if (sampler_params->env_rel < 0)
                                    sampler_params->env_rel = 0;
                            }
                        }
                        break;
                    }
                    }
                }
            }
        } break;
        }
    } else if (*ctx->screen_focus == FOCUS_BOTTOM) {
        switch (ctx->session->touch_screen_view) {
        case VIEW_TOUCH_SETTINGS:
            if (kDown & KEY_LEFT) {
                *ctx->selected_touch_option =
                    (*ctx->selected_touch_option > 0) ? *ctx->selected_touch_option - 1 : 1;
            }
            if (kDown & KEY_RIGHT) {
                *ctx->selected_touch_option =
                    (*ctx->selected_touch_option < 1) ? *ctx->selected_touch_option + 1 : 0;
            }
            if (kDown & KEY_A && *ctx->selected_touch_option == 0) {
                ctx->session->touch_screen_view   = VIEW_TOUCH_CLOCK_SETTINGS;
                *ctx->selected_touch_clock_option = 0;
            } else if (kDown & KEY_A && *ctx->selected_touch_option == 1) {
                ctx->session->touch_screen_view = VIEW_SAMPLE_MANAGER;
                *ctx->selected_sample_row       = 0;
                *ctx->selected_sample_col       = 0;
            }
            if (kDown & KEY_Y && *ctx->selected_touch_option == 0) {
                LightLock_Lock(ctx->clock_lock);
                if (ctx->clock->status == PLAYING) {
                    pauseClock(ctx->clock);
                } else if (ctx->clock->status == PAUSED) {
                    resumeClock(ctx->clock);
                }
                LightLock_Unlock(ctx->clock_lock);
            }
            if (kDown & KEY_X && *ctx->selected_touch_option == 0) {
                LightLock_Lock(ctx->clock_lock);
                if (ctx->clock->status == PLAYING || ctx->clock->status == PAUSED) {
                    stopClock(ctx->clock);
                    LightLock_Lock(ctx->tracks_lock);
                    for (int i = 0; i < N_TRACKS; i++) {
                        if (ctx->tracks[i].sequencer) {
                            ctx->tracks[i].sequencer->cur_step = 0;
                        }
                    }
                    LightLock_Unlock(ctx->tracks_lock);
                } else {
                    startClock(ctx->clock);
                }
                LightLock_Unlock(ctx->clock_lock);
            }
            break;
        case VIEW_TOUCH_CLOCK_SETTINGS:
            if (kDown & KEY_UP) {
                *ctx->selected_touch_clock_option = (*ctx->selected_touch_clock_option > 0)
                                                        ? *ctx->selected_touch_clock_option - 1
                                                        : 2;
            }
            if (kDown & KEY_DOWN) {
                *ctx->selected_touch_clock_option = (*ctx->selected_touch_clock_option < 2)
                                                        ? *ctx->selected_touch_clock_option + 1
                                                        : 0;
            }
            if (kDown & KEY_B) {
                ctx->session->touch_screen_view = VIEW_TOUCH_SETTINGS;
            }
            if (kDown & KEY_A && *ctx->selected_touch_clock_option == 2) {
                ctx->session->touch_screen_view = VIEW_TOUCH_SETTINGS;
            }
            if (handle_continuous_press(kDown, kHeld, now, KEY_LEFT, ctx->left_timer,
                                        ctx->HOLD_DELAY_INITIAL, ctx->HOLD_DELAY_REPEAT)) {
                if (*ctx->selected_touch_clock_option == 0) { // BPM
                    setBpm(ctx->clock, ctx->clock->bpm - 1);
                } else if (*ctx->selected_touch_clock_option == 1) { // Beats per bar
                    setBeatsPerBar(ctx->clock, ctx->clock->barBeats->beats_per_bar - 1);
                }
            }
            if (handle_continuous_press(kDown, kHeld, now, KEY_RIGHT, ctx->right_timer,
                                        ctx->HOLD_DELAY_INITIAL, ctx->HOLD_DELAY_REPEAT)) {
                if (*ctx->selected_touch_clock_option == 0) { // BPM
                    setBpm(ctx->clock, ctx->clock->bpm + 1);
                } else if (*ctx->selected_touch_clock_option == 1) { // Beats per bar
                    setBeatsPerBar(ctx->clock, ctx->clock->barBeats->beats_per_bar + 1);
                }
            }
            break;
        case VIEW_SAMPLE_MANAGER:
            if (*ctx->is_selecting_sample) {
                if (kDown & KEY_UP) {
                    *ctx->selected_sample_browser_index =
                        (*ctx->selected_sample_browser_index > 0)
                            ? *ctx->selected_sample_browser_index - 1
                            : SampleBrowserGetSampleCount(ctx->sample_browser) - 1;
                }
                if (kDown & KEY_DOWN) {
                    *ctx->selected_sample_browser_index =
                        (*ctx->selected_sample_browser_index <
                         SampleBrowserGetSampleCount(ctx->sample_browser) - 1)
                            ? *ctx->selected_sample_browser_index + 1
                            : 0;
                }
                if (kDown & KEY_A) {
                    int sample_slot  = *ctx->selected_sample_row * 4 + *ctx->selected_sample_col;
                    const char *path = SampleBrowserGetSamplePath(
                        ctx->sample_browser, *ctx->selected_sample_browser_index);
                    SampleBankLoadSample(ctx->sample_bank, sample_slot, path);
                    *ctx->is_selecting_sample = false;
                }
                if (kDown & KEY_B) {
                    *ctx->is_selecting_sample = false;
                }
            } else {
                if (kDown & KEY_UP) {
                    *ctx->selected_sample_row =
                        (*ctx->selected_sample_row > 0) ? *ctx->selected_sample_row - 1 : 2;
                }
                if (kDown & KEY_DOWN) {
                    *ctx->selected_sample_row =
                        (*ctx->selected_sample_row < 2) ? *ctx->selected_sample_row + 1 : 0;
                }
                if (kDown & KEY_LEFT) {
                    *ctx->selected_sample_col =
                        (*ctx->selected_sample_col > 0) ? *ctx->selected_sample_col - 1 : 3;
                }
                if (kDown & KEY_RIGHT) {
                    *ctx->selected_sample_col =
                        (*ctx->selected_sample_col < 3) ? *ctx->selected_sample_col + 1 : 0;
                }
                if (kDown & KEY_A) {
                    *ctx->is_selecting_sample           = true;
                    *ctx->selected_sample_browser_index = 0;
                }
                if (kDown & KEY_B) {
                    ctx->session->touch_screen_view = VIEW_TOUCH_SETTINGS;
                }
            }
            break;
        case VIEW_STEP_SETTINGS: {
            int track_idx = *ctx->selected_row - 1;
            if (track_idx < 0)
                break;
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
            int param_count = generateParameterList(track, params_to_show, ctx->sample_bank,
                                                    param_list, MAX_VIEW_PARAMS);

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
            int current_row = (current_col == 0) ? *ctx->selected_step_option
                                                 : *ctx->selected_step_option - num_left;

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
                if (current_col > 0) {
                    current_col = 0;
                    if (current_row >= num_left) {
                        current_row = num_left - 1;
                    }
                }
            }
            if (kDown & KEY_RIGHT) {
                if (current_col < 1) {
                    current_col = 1;
                    if (current_row >= num_right) {
                        current_row = num_right - 1;
                    }
                }
            }

            if (kDown & KEY_X) {
                *ctx->selected_col = (*ctx->selected_col % 16) + 1;
            }

            if (kDown & KEY_B) {
                *ctx->selected_row = (*ctx->selected_row % N_TRACKS) + 1;
            }

            *ctx->selected_step_option = (current_col == 0) ? current_row : current_row + num_left;

            if (kDown & KEY_A) {
                if (track_idx >= 0) {
                    initEditingParams(ctx, track, *ctx->selected_col);
                    if (*ctx->selected_step_option >= 0 &&
                        *ctx->selected_step_option < param_count) {
                        ctx->session->main_screen_view = VIEW_STEP_SETTINGS_EDIT;
                        *ctx->screen_focus             = FOCUS_TOP;
                        *ctx->selected_adsr_option     = 0; // Reset ADSR selection
                    }
                }
            }
        } break;
        default:
            break;
        }
    }
}