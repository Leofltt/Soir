#include "controllers/controller_step_edit.h"
#include "audio_utils.h"
#include "engine_constants.h"
#include "track.h"
#include "synth.h"
#include "samplers.h"
#include "event_queue.h"
#include "ui/ui.h"
#include <string.h>
#include "session.h"
#include "track_parameters.h"
#include "sample_bank.h"
#include "sample.h"
#include "clock.h"
#include <3ds.h>
#include "controllers/session_controller.h"

// Helper for clamping values (assuming it's defined elsewhere or will be defined)
#ifndef clamp
#define clamp(val, min, max) ((val) > (max) ? (max) : ((val) < (min) ? (min) : (val)))
#endif

extern int     hertzToMidi(float hertz);
extern float   midiToHertz(int midiNote);
extern int     SampleBankGetLoadedSampleCount(SampleBank *bank);
extern Sample *SampleBankGetSample(SampleBank *bank, int index);
extern int     generateParameterList(Track *track, TrackParameters *params, SampleBank *sample_bank,
                                     ParameterInfo *list_buffer, int max_params);

void handleInputStepEditView(SessionContext *ctx, u32 kDown, u32 kHeld, u64 now) {
    int track_idx = *ctx->selected_row - 1;
    if (track_idx < 0)
        return;
    Track *track = &ctx->tracks[track_idx];

    if (kDown & KEY_B) { // Cancel
        ctx->session->main_screen_view = VIEW_MAIN;
        *ctx->screen_focus             = FOCUS_BOTTOM;
    } else if (kDown & KEY_A) {
        if (*ctx->selected_col == 0) {
            // Apply to all steps
            for (int i = 0; i < track->sequencer->n_beats * track->sequencer->steps_per_beat; i++) {
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
            memcpy(track->default_parameters, ctx->editing_step_params, sizeof(TrackParameters));
            track->default_parameters->instrument_data = original_instrument_data;

            if (track->instrument_type == SUB_SYNTH) {
                memcpy(track->default_parameters->instrument_data, ctx->editing_subsynth_params,
                       sizeof(SubSynthParameters));
            } else if (track->instrument_type == OPUS_SAMPLER) {
                memcpy(track->default_parameters->instrument_data, ctx->editing_sampler_params,
                       sizeof(OpusSamplerParameters));
            } else if (track->instrument_type == FM_SYNTH) {
                memcpy(track->default_parameters->instrument_data, ctx->editing_fm_synth_params,
                       sizeof(FMSynthParameters));
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
        bool          param_changed = false;
        ParameterInfo param_list[MAX_VIEW_PARAMS];
        int param_count = generateParameterList(track, ctx->editing_step_params, ctx->sample_bank,
                                                param_list, MAX_VIEW_PARAMS);

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

                                                ctx->HOLD_DELAY_INITIAL, ctx->HOLD_DELAY_REPEAT))

                        fm_params->mod_depth += 1.0f;

                    if (handle_continuous_press(kDown, kHeld, now, KEY_DOWN, ctx->down_timer,

                                                ctx->HOLD_DELAY_INITIAL, ctx->HOLD_DELAY_REPEAT))

                        fm_params->mod_depth -= 1.0f;

                    fm_params->mod_depth = clamp(fm_params->mod_depth, 0.0f, 1000.0f);

                } else {
                    float *value_ptr = NULL;

                    if (strcmp(param_to_edit->label, "Volume") == 0)

                        value_ptr = &ctx->editing_step_params->volume;

                    else if (strcmp(param_to_edit->label, "Mod Index") == 0)

                        value_ptr =

                            &((FMSynthParameters *) ctx->editing_fm_synth_params)->mod_index;

                    else if (strcmp(param_to_edit->label, "Start Pos") == 0) {
                        OpusSamplerParameters *sampler_params = ctx->editing_sampler_params;

                        Sample *sample =

                            SampleBankGetSample(ctx->sample_bank, sampler_params->sample_index);

                        if (sample && sample->pcm_length > 0) {
                            float pos = (float) sampler_params->start_position / sample->pcm_length;

                            if (handle_continuous_press(kDown, kHeld, now, KEY_UP, ctx->up_timer,

                                                        ctx->HOLD_DELAY_INITIAL,

                                                        ctx->HOLD_DELAY_REPEAT))

                                pos += 0.01f;

                            if (handle_continuous_press(kDown, kHeld, now, KEY_DOWN,

                                                        ctx->down_timer, ctx->HOLD_DELAY_INITIAL,

                                                        ctx->HOLD_DELAY_REPEAT))

                                pos -= 0.01f;

                            pos = clamp(pos, 0.0f, 1.0f);

                            sampler_params->start_position = pos * sample->pcm_length;
                        }
                    }

                    if (value_ptr) {
                        if (handle_continuous_press(kDown, kHeld, now, KEY_DOWN, ctx->down_timer,

                                                    ctx->HOLD_DELAY_INITIAL,

                                                    ctx->HOLD_DELAY_REPEAT))

                            *value_ptr -= 0.1f;

                        if (handle_continuous_press(kDown, kHeld, now, KEY_UP, ctx->up_timer,

                                                    ctx->HOLD_DELAY_INITIAL,

                                                    ctx->HOLD_DELAY_REPEAT))

                            *value_ptr += 0.1f;

                        *value_ptr = clamp(*value_ptr, 0.0f, 1.0f);
                    }
                }

                break;
            }

            case PARAM_TYPE_FLOAT_N1_1: {
                if (strcmp(param_to_edit->label, "Pan") == 0) {
                    if (handle_continuous_press(kDown, kHeld, now, KEY_DOWN, ctx->down_timer,

                                                ctx->HOLD_DELAY_INITIAL, ctx->HOLD_DELAY_REPEAT))

                        ctx->editing_step_params->pan -= 0.1f;

                    if (handle_continuous_press(kDown, kHeld, now, KEY_UP, ctx->up_timer,

                                                ctx->HOLD_DELAY_INITIAL, ctx->HOLD_DELAY_REPEAT))

                        ctx->editing_step_params->pan += 0.1f;

                    ctx->editing_step_params->pan =

                        clamp(ctx->editing_step_params->pan, -1.0f, 1.0f);
                }

                break;
            }

            case PARAM_TYPE_HZ: {
                if (strcmp(param_to_edit->label, "Filter Cf") == 0) {
                    if (handle_continuous_press(kDown, kHeld, now, KEY_UP, ctx->up_timer,

                                                ctx->HOLD_DELAY_INITIAL, ctx->HOLD_DELAY_REPEAT))

                        ctx->editing_step_params->ndsp_filter_cutoff += 100;

                    if (handle_continuous_press(kDown, kHeld, now, KEY_DOWN, ctx->down_timer,

                                                ctx->HOLD_DELAY_INITIAL, ctx->HOLD_DELAY_REPEAT))

                        ctx->editing_step_params->ndsp_filter_cutoff -= 100;

                    ctx->editing_step_params->ndsp_filter_cutoff =

                        clamp(ctx->editing_step_params->ndsp_filter_cutoff, 0, 20000);
                }

                break;
            }

            case PARAM_TYPE_MIDI_NOTE: {
                float *freq_ptr = NULL;

                if (track->instrument_type == SUB_SYNTH)

                    freq_ptr = &((SubSynthParameters *) ctx->editing_subsynth_params)->osc_freq;

                else if (track->instrument_type == FM_SYNTH)

                    freq_ptr = &((FMSynthParameters *) ctx->editing_fm_synth_params)->carrier_freq;

                if (freq_ptr) {
                    int midi_note = hertzToMidi(*freq_ptr);

                    if (handle_continuous_press(kDown, kHeld, now, KEY_UP, ctx->up_timer,

                                                ctx->HOLD_DELAY_INITIAL, ctx->HOLD_DELAY_REPEAT))

                        midi_note++;

                    if (handle_continuous_press(kDown, kHeld, now, KEY_DOWN, ctx->down_timer,

                                                ctx->HOLD_DELAY_INITIAL, ctx->HOLD_DELAY_REPEAT))

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

                                                ctx->HOLD_DELAY_INITIAL, ctx->HOLD_DELAY_REPEAT))

                        fm_params->mod_freq_ratio += 0.1f;

                    if (handle_continuous_press(kDown, kHeld, now, KEY_DOWN, ctx->down_timer,

                                                ctx->HOLD_DELAY_INITIAL, ctx->HOLD_DELAY_REPEAT))

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

                            (synth_params->osc_waveform - 1 + WAVEFORM_COUNT) % WAVEFORM_COUNT;
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

                    value_ptr = &((SubSynthParameters *) ctx->editing_subsynth_params)->env_dur;

                else if (track->instrument_type == OPUS_SAMPLER)

                    value_ptr = &((OpusSamplerParameters *) ctx->editing_sampler_params)->env_dur;

                else if (track->instrument_type == FM_SYNTH)

                    value_ptr = &((FMSynthParameters *) ctx->editing_fm_synth_params)->env_dur;

                if (value_ptr) {
                    if (handle_continuous_press(kDown, kHeld, now, KEY_DOWN, ctx->down_timer,

                                                ctx->HOLD_DELAY_INITIAL,

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

                    *ctx->selected_adsr_option =

                        (*ctx->selected_adsr_option > 0) ? *ctx->selected_adsr_option - 1 : 3;

                if (kDown & KEY_RIGHT)

                    *ctx->selected_adsr_option =

                        (*ctx->selected_adsr_option < 3) ? *ctx->selected_adsr_option + 1 : 0;

                if (track->instrument_type == SUB_SYNTH) {
                    SubSynthParameters *synth_params = ctx->editing_subsynth_params;

                    if (*ctx->selected_adsr_option == 0) { // Attack

                        if (handle_continuous_press(kDown, kHeld, now, KEY_UP, ctx->up_timer,

                                                    ctx->HOLD_DELAY_INITIAL,

                                                    ctx->HOLD_DELAY_REPEAT))

                            synth_params->env_atk += 10;

                        if (handle_continuous_press(kDown, kHeld, now, KEY_DOWN, ctx->down_timer,

                                                    ctx->HOLD_DELAY_INITIAL,

                                                    ctx->HOLD_DELAY_REPEAT))

                            synth_params->env_atk -= 10;

                        if (synth_params->env_atk < 0)

                            synth_params->env_atk = 0;

                    } else if (*ctx->selected_adsr_option == 1) { // Decay

                        if (handle_continuous_press(kDown, kHeld, now, KEY_UP, ctx->up_timer,

                                                    ctx->HOLD_DELAY_INITIAL,

                                                    ctx->HOLD_DELAY_REPEAT))

                            synth_params->env_dec += 10;

                        if (handle_continuous_press(kDown, kHeld, now, KEY_DOWN, ctx->down_timer,

                                                    ctx->HOLD_DELAY_INITIAL,

                                                    ctx->HOLD_DELAY_REPEAT))

                            synth_params->env_dec -= 10;

                        if (synth_params->env_dec < 0)

                            synth_params->env_dec = 0;

                    } else if (*ctx->selected_adsr_option == 2) { // Sustain

                        if (handle_continuous_press(kDown, kHeld, now, KEY_UP, ctx->up_timer,

                                                    ctx->HOLD_DELAY_INITIAL,

                                                    ctx->HOLD_DELAY_REPEAT))

                            synth_params->env_sus_level += 0.1f;

                        if (handle_continuous_press(kDown, kHeld, now, KEY_DOWN, ctx->down_timer,

                                                    ctx->HOLD_DELAY_INITIAL,

                                                    ctx->HOLD_DELAY_REPEAT))

                            synth_params->env_sus_level -= 0.1f;

                        synth_params->env_sus_level =

                            clamp(synth_params->env_sus_level, 0.0f, 1.0f);

                    } else if (*ctx->selected_adsr_option == 3) { // Release

                        if (handle_continuous_press(kDown, kHeld, now, KEY_UP, ctx->up_timer,

                                                    ctx->HOLD_DELAY_INITIAL,

                                                    ctx->HOLD_DELAY_REPEAT))

                            synth_params->env_rel += 10;

                        if (handle_continuous_press(kDown, kHeld, now, KEY_DOWN, ctx->down_timer,

                                                    ctx->HOLD_DELAY_INITIAL,

                                                    ctx->HOLD_DELAY_REPEAT))

                            synth_params->env_rel -= 10;

                        if (synth_params->env_rel < 0)

                            synth_params->env_rel = 0;
                    }

                } else if (track->instrument_type == FM_SYNTH) {
                    FMSynthParameters *fm_synth_params = ctx->editing_fm_synth_params;

                    if (strcmp(param_to_edit->label, "Car Env") == 0) {
                        if (*ctx->selected_adsr_option == 0) { // Attack

                            if (handle_continuous_press(kDown, kHeld, now, KEY_UP, ctx->up_timer,

                                                        ctx->HOLD_DELAY_INITIAL,

                                                        ctx->HOLD_DELAY_REPEAT))

                                fm_synth_params->carrier_env_atk += 10;

                            if (handle_continuous_press(kDown, kHeld, now, KEY_DOWN,

                                                        ctx->down_timer, ctx->HOLD_DELAY_INITIAL,

                                                        ctx->HOLD_DELAY_REPEAT))

                                fm_synth_params->carrier_env_atk -= 10;

                            if (fm_synth_params->carrier_env_atk < 0)

                                fm_synth_params->carrier_env_atk = 0;

                        } else if (*ctx->selected_adsr_option == 1) { // Decay

                            if (handle_continuous_press(kDown, kHeld, now, KEY_UP, ctx->up_timer,

                                                        ctx->HOLD_DELAY_INITIAL,

                                                        ctx->HOLD_DELAY_REPEAT))

                                fm_synth_params->carrier_env_dec += 10;

                            if (handle_continuous_press(kDown, kHeld, now, KEY_DOWN,

                                                        ctx->down_timer, ctx->HOLD_DELAY_INITIAL,

                                                        ctx->HOLD_DELAY_REPEAT))

                                fm_synth_params->carrier_env_dec -= 10;

                            if (fm_synth_params->carrier_env_dec < 0)

                                fm_synth_params->carrier_env_dec = 0;

                        } else if (*ctx->selected_adsr_option == 2) { // Sustain

                            if (handle_continuous_press(kDown, kHeld, now, KEY_UP, ctx->up_timer,

                                                        ctx->HOLD_DELAY_INITIAL,

                                                        ctx->HOLD_DELAY_REPEAT))

                                fm_synth_params->carrier_env_sus_level += 0.1f;

                            if (handle_continuous_press(kDown, kHeld, now, KEY_DOWN,

                                                        ctx->down_timer, ctx->HOLD_DELAY_INITIAL,

                                                        ctx->HOLD_DELAY_REPEAT))

                                fm_synth_params->carrier_env_sus_level -= 0.1f;

                            fm_synth_params->carrier_env_sus_level =

                                clamp(fm_synth_params->carrier_env_sus_level, 0.0f, 1.0f);

                        } else if (*ctx->selected_adsr_option == 3) { // Release

                            if (handle_continuous_press(kDown, kHeld, now, KEY_UP, ctx->up_timer,

                                                        ctx->HOLD_DELAY_INITIAL,

                                                        ctx->HOLD_DELAY_REPEAT))

                                fm_synth_params->carrier_env_rel += 10;

                            if (handle_continuous_press(kDown, kHeld, now, KEY_DOWN,

                                                        ctx->down_timer, ctx->HOLD_DELAY_INITIAL,

                                                        ctx->HOLD_DELAY_REPEAT))

                                fm_synth_params->carrier_env_rel -= 10;

                            if (fm_synth_params->carrier_env_rel < 0)

                                fm_synth_params->carrier_env_rel = 0;
                        }

                    } else { // Mod Env

                        if (*ctx->selected_adsr_option == 0) { // Attack

                            if (handle_continuous_press(kDown, kHeld, now, KEY_UP, ctx->up_timer,

                                                        ctx->HOLD_DELAY_INITIAL,

                                                        ctx->HOLD_DELAY_REPEAT))

                                fm_synth_params->mod_env_atk += 10;

                            if (handle_continuous_press(kDown, kHeld, now, KEY_DOWN,

                                                        ctx->down_timer, ctx->HOLD_DELAY_INITIAL,

                                                        ctx->HOLD_DELAY_REPEAT))

                                fm_synth_params->mod_env_atk -= 10;

                            if (fm_synth_params->mod_env_atk < 0)

                                fm_synth_params->mod_env_atk = 0;

                        } else if (*ctx->selected_adsr_option == 1) { // Decay

                            if (handle_continuous_press(kDown, kHeld, now, KEY_UP, ctx->up_timer,

                                                        ctx->HOLD_DELAY_INITIAL,

                                                        ctx->HOLD_DELAY_REPEAT))

                                fm_synth_params->mod_env_dec += 10;

                            if (handle_continuous_press(kDown, kHeld, now, KEY_DOWN,

                                                        ctx->down_timer, ctx->HOLD_DELAY_INITIAL,

                                                        ctx->HOLD_DELAY_REPEAT))

                                fm_synth_params->mod_env_dec -= 10;

                            if (fm_synth_params->mod_env_dec < 0)

                                fm_synth_params->mod_env_dec = 0;

                        } else if (*ctx->selected_adsr_option == 2) { // Sustain

                            if (handle_continuous_press(kDown, kHeld, now, KEY_UP, ctx->up_timer,

                                                        ctx->HOLD_DELAY_INITIAL,

                                                        ctx->HOLD_DELAY_REPEAT))

                                fm_synth_params->mod_env_sus_level += 0.1f;

                            if (handle_continuous_press(kDown, kHeld, now, KEY_DOWN,

                                                        ctx->down_timer, ctx->HOLD_DELAY_INITIAL,

                                                        ctx->HOLD_DELAY_REPEAT))

                                fm_synth_params->mod_env_sus_level -= 0.1f;

                            fm_synth_params->mod_env_sus_level =

                                clamp(fm_synth_params->mod_env_sus_level, 0.0f, 1.0f);

                        } else if (*ctx->selected_adsr_option == 3) { // Release

                            if (handle_continuous_press(kDown, kHeld, now, KEY_UP, ctx->up_timer,

                                                        ctx->HOLD_DELAY_INITIAL,

                                                        ctx->HOLD_DELAY_REPEAT))

                                fm_synth_params->mod_env_rel += 10;

                            if (handle_continuous_press(kDown, kHeld, now, KEY_DOWN,

                                                        ctx->down_timer, ctx->HOLD_DELAY_INITIAL,

                                                        ctx->HOLD_DELAY_REPEAT))

                                fm_synth_params->mod_env_rel -= 10;

                            if (fm_synth_params->mod_env_rel < 0)

                                fm_synth_params->mod_env_rel = 0;
                        }
                    }

                } else if (track->instrument_type == OPUS_SAMPLER) {
                    OpusSamplerParameters *sampler_params = ctx->editing_sampler_params;

                    if (*ctx->selected_adsr_option == 0) { // Attack

                        if (handle_continuous_press(kDown, kHeld, now, KEY_UP, ctx->up_timer,

                                                    ctx->HOLD_DELAY_INITIAL,

                                                    ctx->HOLD_DELAY_REPEAT))

                            sampler_params->env_atk += 10;

                        if (handle_continuous_press(kDown, kHeld, now, KEY_DOWN, ctx->down_timer,

                                                    ctx->HOLD_DELAY_INITIAL,

                                                    ctx->HOLD_DELAY_REPEAT))

                            sampler_params->env_atk -= 10;

                        if (sampler_params->env_atk < 0)

                            sampler_params->env_atk = 0;

                    } else if (*ctx->selected_adsr_option == 1) { // Decay

                        if (handle_continuous_press(kDown, kHeld, now, KEY_UP, ctx->up_timer,

                                                    ctx->HOLD_DELAY_INITIAL,

                                                    ctx->HOLD_DELAY_REPEAT))

                            sampler_params->env_dec += 10;

                        if (handle_continuous_press(kDown, kHeld, now, KEY_DOWN, ctx->down_timer,

                                                    ctx->HOLD_DELAY_INITIAL,

                                                    ctx->HOLD_DELAY_REPEAT))

                            sampler_params->env_dec -= 10;

                        if (sampler_params->env_dec < 0)

                            sampler_params->env_dec = 0;

                    } else if (*ctx->selected_adsr_option == 2) { // Sustain

                        if (handle_continuous_press(kDown, kHeld, now, KEY_UP, ctx->up_timer,

                                                    ctx->HOLD_DELAY_INITIAL,

                                                    ctx->HOLD_DELAY_REPEAT))

                            sampler_params->env_sus_level += 0.1f;

                        if (handle_continuous_press(kDown, kHeld, now, KEY_DOWN, ctx->down_timer,

                                                    ctx->HOLD_DELAY_INITIAL,

                                                    ctx->HOLD_DELAY_REPEAT))

                            sampler_params->env_sus_level -= 0.1f;

                        sampler_params->env_sus_level =

                            clamp(sampler_params->env_sus_level, 0.0f, 1.0f);

                    } else if (*ctx->selected_adsr_option == 3) { // Release

                        if (handle_continuous_press(kDown, kHeld, now, KEY_UP, ctx->up_timer,

                                                    ctx->HOLD_DELAY_INITIAL,

                                                    ctx->HOLD_DELAY_REPEAT))

                            sampler_params->env_rel += 10;

                        if (handle_continuous_press(kDown, kHeld, now, KEY_DOWN, ctx->down_timer,

                                                    ctx->HOLD_DELAY_INITIAL,

                                                    ctx->HOLD_DELAY_REPEAT))

                            sampler_params->env_rel -= 10;

                        if (sampler_params->env_rel < 0)

                            sampler_params->env_rel = 0;
                    }
                }

                break;
            }
            }
        }

        // Update ctx->editing_step_params->instrument_data with the current instrument-specific
        // parameters

        if (track->instrument_type == SUB_SYNTH) {
            memcpy(ctx->editing_step_params->instrument_data, ctx->editing_subsynth_params,

                   sizeof(SubSynthParameters));

        } else if (track->instrument_type == OPUS_SAMPLER) {
            memcpy(ctx->editing_step_params->instrument_data, ctx->editing_sampler_params,

                   sizeof(OpusSamplerParameters));

        } else if (track->instrument_type == FM_SYNTH) {
            memcpy(ctx->editing_step_params->instrument_data, ctx->editing_fm_synth_params,

                   sizeof(FMSynthParameters));
        }

        if (param_changed) {
            param_count = generateParameterList(track, ctx->editing_step_params, ctx->sample_bank,

                                                param_list, MAX_VIEW_PARAMS);

            drawStepSettingsEditView(track, ctx->editing_step_params, *ctx->selected_step_option,

                                     *ctx->selected_adsr_option, ctx->sample_bank);
        }
    }
}