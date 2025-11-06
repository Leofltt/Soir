#include "views.h"
#include "ui_constants.h"
#include "engine_constants.h"
#include "filters.h"
#include "sample_bank.h"
#include "sample.h"
#include "audio_utils.h"
#include "polybleposc.h"
#include <citro2d.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

extern C2D_Font    font_angular;
extern C2D_Font    font_heavy;
extern C2D_TextBuf text_buf;
extern C2D_Text    text_obj;

static const char *playback_mode_names[] = { "One Shot", "Loop" };

int generateParameterList(Track *track, TrackParameters *params, SampleBank *sample_bank,
                          ParameterInfo *list_buffer, int max_params) {
    int id = 0;

    // Column 0: Common Parameters
    list_buffer[id] = (ParameterInfo) { .label         = "Volume",
                                        .unique_id     = id,
                                        .column        = 0,
                                        .row_in_column = 0,
                                        .type          = PARAM_TYPE_FLOAT_0_1 };
    snprintf(list_buffer[id].value_string, sizeof(list_buffer[id].value_string), "%.2f",
             params->volume);
    id++;

    list_buffer[id] = (ParameterInfo) { .label         = "Pan",
                                        .unique_id     = id,
                                        .column        = 0,
                                        .row_in_column = 1,
                                        .type          = PARAM_TYPE_FLOAT_N1_1 };
    snprintf(list_buffer[id].value_string, sizeof(list_buffer[id].value_string), "%.2f",
             params->pan);
    id++;

    list_buffer[id] = (ParameterInfo) { .label         = "Filter Cf",
                                        .unique_id     = id,
                                        .column        = 0,
                                        .row_in_column = 2,
                                        .type          = PARAM_TYPE_HZ };
    snprintf(list_buffer[id].value_string, sizeof(list_buffer[id].value_string), "%.0f",
             params->ndsp_filter_cutoff);
    id++;

    list_buffer[id] = (ParameterInfo) { .label         = "Filter Type",
                                        .unique_id     = id,
                                        .column        = 0,
                                        .row_in_column = 3,
                                        .type          = PARAM_TYPE_FILTER_TYPE };
    snprintf(list_buffer[id].value_string, sizeof(list_buffer[id].value_string), "%s",
             ndsp_biquad_filter_names[params->ndsp_filter_type]);
    id++;

    // Column 1: Instrument-specific Parameters
    switch (track->instrument_type) {
    case SUB_SYNTH: {
        SubSynthParameters *synth_params = (SubSynthParameters *) params->instrument_data;
        list_buffer[id]                  = (ParameterInfo) { .label         = "MIDI Note",
                                                             .unique_id     = id,
                                                             .column        = 1,
                                                             .row_in_column = 0,
                                                             .type          = PARAM_TYPE_MIDI_NOTE };
        snprintf(list_buffer[id].value_string, sizeof(list_buffer[id].value_string), "%d",
                 hertzToMidi(synth_params->osc_freq));
        id++;

        list_buffer[id] = (ParameterInfo) { .label         = "Waveform",
                                            .unique_id     = id,
                                            .column        = 1,
                                            .row_in_column = 1,
                                            .type          = PARAM_TYPE_WAVEFORM };
        snprintf(list_buffer[id].value_string, sizeof(list_buffer[id].value_string), "%s",
                 waveform_names[synth_params->osc_waveform]);
        id++;

        list_buffer[id] = (ParameterInfo) { .label         = "Envelope",
                                            .unique_id     = id,
                                            .column        = 1,
                                            .row_in_column = 2,
                                            .type          = PARAM_TYPE_ENVELOPE_BUTTON };
        snprintf(list_buffer[id].value_string, sizeof(list_buffer[id].value_string), "Edit");
        id++;

        list_buffer[id] = (ParameterInfo) { .label         = "Env Dur",
                                            .unique_id     = id,
                                            .column        = 1,
                                            .row_in_column = 3,
                                            .type          = PARAM_TYPE_INT };
        snprintf(list_buffer[id].value_string, sizeof(list_buffer[id].value_string), "%d",
                 synth_params->env_dur);
        id++;
        break;
    }
    case FM_SYNTH: {
        FMSynthParameters *fm_params = (FMSynthParameters *) params->instrument_data;
        list_buffer[id]              = (ParameterInfo) { .label         = "Mod Depth",
                                                         .unique_id     = id,
                                                         .column        = 0,
                                                         .row_in_column = 4,
                                                         .type          = PARAM_TYPE_FLOAT_0_1 };
        snprintf(list_buffer[id].value_string, sizeof(list_buffer[id].value_string), "%.2f",
                 fm_params->mod_depth);
        id++;

        list_buffer[id] = (ParameterInfo) { .label         = "Mod Index",
                                            .unique_id     = id,
                                            .column        = 0,
                                            .row_in_column = 5,
                                            .type          = PARAM_TYPE_FLOAT_0_1 };
        snprintf(list_buffer[id].value_string, sizeof(list_buffer[id].value_string), "%.1f",
                 fm_params->mod_index);
        id++;

        list_buffer[id] = (ParameterInfo) { .label         = "MIDI Note",
                                            .unique_id     = id,
                                            .column        = 1,
                                            .row_in_column = 0,
                                            .type          = PARAM_TYPE_MIDI_NOTE };
        snprintf(list_buffer[id].value_string, sizeof(list_buffer[id].value_string), "%d",
                 hertzToMidi(fm_params->carrier_freq));
        id++;

        list_buffer[id] = (ParameterInfo) { .label         = "Mod Ratio",
                                            .unique_id     = id,
                                            .column        = 1,
                                            .row_in_column = 1,
                                            .type          = PARAM_TYPE_MOD_RATIO };
        snprintf(list_buffer[id].value_string, sizeof(list_buffer[id].value_string), "%.1f",
                 fm_params->mod_freq_ratio);
        id++;

        list_buffer[id] = (ParameterInfo) { .label         = "Car Env",
                                            .unique_id     = id,
                                            .column        = 1,
                                            .row_in_column = 2,
                                            .type          = PARAM_TYPE_ENVELOPE_BUTTON };
        snprintf(list_buffer[id].value_string, sizeof(list_buffer[id].value_string), "Edit");
        id++;

        list_buffer[id] = (ParameterInfo) { .label         = "Mod Env",
                                            .unique_id     = id,
                                            .column        = 1,
                                            .row_in_column = 3,
                                            .type          = PARAM_TYPE_ENVELOPE_BUTTON };
        snprintf(list_buffer[id].value_string, sizeof(list_buffer[id].value_string), "Edit");
        id++;

        list_buffer[id] = (ParameterInfo) { .label         = "Env Dur",
                                            .unique_id     = id,
                                            .column        = 1,
                                            .row_in_column = 4,
                                            .type          = PARAM_TYPE_INT };
        snprintf(list_buffer[id].value_string, sizeof(list_buffer[id].value_string), "%d",
                 fm_params->env_dur);
        id++;
        break;
    }
    case OPUS_SAMPLER: {
        OpusSamplerParameters *sampler_params = (OpusSamplerParameters *) params->instrument_data;
        list_buffer[id]                       = (ParameterInfo) { .label         = "Sample",
                                                                  .unique_id     = id,
                                                                  .column        = 1,
                                                                  .row_in_column = 0,
                                                                  .type          = PARAM_TYPE_SAMPLE_INDEX };
        snprintf(list_buffer[id].value_string, sizeof(list_buffer[id].value_string), "%s",
                 SampleBankGetSampleName(sample_bank, sampler_params->sample_index));
        id++;

        list_buffer[id] = (ParameterInfo) { .label         = "Pb Mode",
                                            .unique_id     = id,
                                            .column        = 1,
                                            .row_in_column = 1,
                                            .type          = PARAM_TYPE_PLAYBACK_MODE };
        snprintf(list_buffer[id].value_string, sizeof(list_buffer[id].value_string), "%s",
                 playback_mode_names[sampler_params->playback_mode]);
        id++;

        list_buffer[id] = (ParameterInfo) { .label         = "Start Pos",
                                            .unique_id     = id,
                                            .column        = 1,
                                            .row_in_column = 2,
                                            .type          = PARAM_TYPE_FLOAT_0_1 };
        Sample *sample  = SampleBankGetSample(sample_bank, sampler_params->sample_index);
        float   start_pos_normalized = 0.0f;
        if (sample && sample->pcm_length > 0) {
            start_pos_normalized = (float) sampler_params->start_position / sample->pcm_length;
        }
        snprintf(list_buffer[id].value_string, sizeof(list_buffer[id].value_string), "%.2f",
                 start_pos_normalized);
        id++;

        list_buffer[id] = (ParameterInfo) { .label         = "Envelope",
                                            .unique_id     = id,
                                            .column        = 1,
                                            .row_in_column = 3,
                                            .type          = PARAM_TYPE_ENVELOPE_BUTTON };
        snprintf(list_buffer[id].value_string, sizeof(list_buffer[id].value_string), "Edit");
        id++;

        list_buffer[id] = (ParameterInfo) { .label         = "Env Dur",
                                            .unique_id     = id,
                                            .column        = 1,
                                            .row_in_column = 4,
                                            .type          = PARAM_TYPE_INT };
        snprintf(list_buffer[id].value_string, sizeof(list_buffer[id].value_string), "%d",
                 sampler_params->env_dur);
        id++;
        break;
    }
    }

    return id;
}

void drawStepSettingsView(Session *session, Track *tracks, int selected_row, int selected_col,
                          int selected_step_option, SampleBank *sample_bank, ScreenFocus focus) {
    if (selected_row == 0) {
        C2D_TextBufClear(text_buf);
        C2D_TextFontParse(&text_obj, font_angular, text_buf, "No Track Selected");
        C2D_TextOptimize(&text_obj);

        float text_width, text_height;
        C2D_TextGetDimensions(&text_obj, 0.5f, 0.5f, &text_width, &text_height);

        float text_x = (BOTTOM_SCREEN_WIDTH - text_width) / 2;
        float text_y = (SCREEN_HEIGHT - text_height) / 2;

        C2D_DrawText(&text_obj, C2D_WithColor, text_x, text_y, 0.0f, 0.5f, 0.5f, CLR_LIGHT_GRAY);
        return;
    }

    int    track_idx = selected_row - 1;
    Track *track     = &tracks[track_idx];

    TrackParameters *params;
    bool             is_all_steps = (selected_col == 0);

    if (is_all_steps) {
        params = track->default_parameters;
    } else {
        int step_idx = selected_col - 1;
        params       = track->sequencer->steps[step_idx].data;
    }

    ParameterInfo param_list[MAX_VIEW_PARAMS];
    int           param_count =
        generateParameterList(track, params, sample_bank, param_list, MAX_VIEW_PARAMS);

    const char *instrument_name = "";
    if (track->instrument_type == SUB_SYNTH) {
        instrument_name = "Synth";
    } else if (track->instrument_type == FM_SYNTH) {
        instrument_name = "FM Synth";
    } else if (track->instrument_type == OPUS_SAMPLER) {
        instrument_name = "Sampler";
    }

    C2D_TextBufClear(text_buf);
    C2D_TextFontParse(&text_obj, font_angular, text_buf, instrument_name);
    C2D_TextOptimize(&text_obj);
    C2D_DrawText(&text_obj, C2D_WithColor, 10, 10, 0.0f, 0.5f, 0.5f, CLR_LIGHT_GRAY);

    float cell_width  = 140;
    float cell_height = 20;
    float padding     = 5;

    u32  base_bg_color, base_text_color, border_color;
    bool is_active;
    if (is_all_steps) {
        is_active = !track->is_muted;
    } else {
        is_active = track->sequencer->steps[selected_col - 1].active;
    }

    if (is_active) {
        base_bg_color   = CLR_LIGHT_GRAY;
        base_text_color = CLR_BLACK;
        border_color    = CLR_BLACK;
    } else {
        base_bg_color   = CLR_BLACK;
        base_text_color = CLR_LIGHT_GRAY;
        border_color    = CLR_LIGHT_GRAY;
    }

    for (int i = 0; i < param_count; i++) {
        ParameterInfo *p           = &param_list[i];
        float          x           = (p->column == 0) ? 10 : 160;
        float          y           = 30 + p->row_in_column * (cell_height + padding);
        bool           is_selected = (p->unique_id == selected_step_option);

        u32 bg_color, text_color;
        if (is_selected) {
            if (focus == FOCUS_BOTTOM) {
                bg_color   = CLR_YELLOW;
                text_color = CLR_BLACK;
            } else {
                bg_color   = base_bg_color;
                text_color = base_text_color;
            }
        } else {
            bg_color   = base_bg_color;
            text_color = base_text_color;
        }

        C2D_DrawRectangle(x, y, 0, cell_width, cell_height, bg_color, bg_color, bg_color, bg_color);

        u32 current_border_color =
            (is_selected && focus != FOCUS_BOTTOM) ? CLR_YELLOW : border_color;
        C2D_DrawRectangle(x, y, 0, cell_width, 1, current_border_color, current_border_color,
                          current_border_color, current_border_color);
        C2D_DrawRectangle(x, y + cell_height - 1, 0, cell_width, 1, current_border_color,
                          current_border_color, current_border_color, current_border_color);
        C2D_DrawRectangle(x, y, 0, 1, cell_height, current_border_color, current_border_color,
                          current_border_color, current_border_color);
        C2D_DrawRectangle(x + cell_width - 1, y, 0, 1, cell_height, current_border_color,
                          current_border_color, current_border_color, current_border_color);

        C2D_TextBufClear(text_buf);
        char buffer[128];
        snprintf(buffer, sizeof(buffer), "%s: %s", p->label, p->value_string);
        C2D_TextFontParse(&text_obj, font_angular, text_buf, buffer);
        C2D_TextOptimize(&text_obj);
        C2D_DrawText(&text_obj, C2D_WithColor, x + padding, y + padding, 0.0f, 0.3f, 0.3f,
                     text_color);
    }

    // Track and Step Info
    char buffer[128];
    if (is_all_steps) {
        snprintf(buffer, sizeof(buffer), "Tr: %d | St: All", track_idx + 1);
    } else {
        snprintf(buffer, sizeof(buffer), "Tr: %d | St: %d", track_idx + 1, selected_col);
    }
    C2D_TextBufClear(text_buf);
    C2D_TextFontParse(&text_obj, font_angular, text_buf, buffer);
    C2D_TextOptimize(&text_obj);
    C2D_DrawText(&text_obj, C2D_WithColor, 200, 10, 0.0f, 0.4f, 0.4f, CLR_LIGHT_GRAY);
}

void drawStepSettingsEditView(Track *track, TrackParameters *params, int selected_step_option,
                              int selected_adsr_option, SampleBank *sample_bank) {
    C2D_DrawRectangle(0, 0, 0, TOP_SCREEN_WIDTH, SCREEN_HEIGHT, C2D_Color32(0, 0, 0, 128),
                      C2D_Color32(0, 0, 0, 128), C2D_Color32(0, 0, 0, 128),
                      C2D_Color32(0, 0, 0, 128));

    // Menu box
    float menu_width  = 300;
    float menu_height = 100;
    float menu_x      = (TOP_SCREEN_WIDTH - menu_width) / 2;
    float menu_y      = (SCREEN_HEIGHT - menu_height) / 2;
    C2D_DrawRectangle(menu_x, menu_y, 0, menu_width, menu_height, CLR_BLACK, CLR_BLACK, CLR_BLACK,
                      CLR_BLACK);
    // Border
    C2D_DrawRectangle(menu_x, menu_y, 0, menu_width, 1, CLR_LIGHT_GRAY, CLR_LIGHT_GRAY,
                      CLR_LIGHT_GRAY, CLR_LIGHT_GRAY);
    C2D_DrawRectangle(menu_x, menu_y + menu_height - 1, 0, menu_width, 1, CLR_LIGHT_GRAY,
                      CLR_LIGHT_GRAY, CLR_LIGHT_GRAY, CLR_LIGHT_GRAY);
    C2D_DrawRectangle(menu_x, menu_y, 0, 1, menu_height, CLR_LIGHT_GRAY, CLR_LIGHT_GRAY,
                      CLR_LIGHT_GRAY, CLR_LIGHT_GRAY);
    C2D_DrawRectangle(menu_x + menu_width - 1, menu_y, 0, 1, menu_height, CLR_LIGHT_GRAY,
                      CLR_LIGHT_GRAY, CLR_LIGHT_GRAY, CLR_LIGHT_GRAY);

    ParameterInfo param_list[MAX_VIEW_PARAMS];
    int           param_count =
        generateParameterList(track, params, sample_bank, param_list, MAX_VIEW_PARAMS);

    ParameterInfo *param_to_edit = NULL;
    for (int i = 0; i < param_count; i++) {
        if (param_list[i].unique_id == selected_step_option) {
            param_to_edit = &param_list[i];
            break;
        }
    }

    if (param_to_edit) {
        C2D_TextBufClear(text_buf);
        char text[128];

        switch (param_to_edit->type) {
        case PARAM_TYPE_ENVELOPE_BUTTON: {
            const char *labels[] = { "A", "D", "S", "R" };
            float       values[4];
            char        value_str[4][16];

            if (track->instrument_type == SUB_SYNTH) {
                SubSynthParameters *synth_params = (SubSynthParameters *) params->instrument_data;
                values[0]                        = synth_params->env_atk;
                values[1]                        = synth_params->env_dec;
                values[2]                        = synth_params->env_sus_level;
                values[3]                        = synth_params->env_rel;
            } else if (track->instrument_type == FM_SYNTH) {
                FMSynthParameters *fm_synth_params = (FMSynthParameters *) params->instrument_data;
                if (fm_synth_params) {
                    if (strcmp(param_to_edit->label, "Car Env") == 0) {
                        values[0] = fm_synth_params->carrier_env_atk;
                        values[1] = fm_synth_params->carrier_env_dec;
                        values[2] = fm_synth_params->carrier_env_sus_level;
                        values[3] = fm_synth_params->carrier_env_rel;
                    } else { // Modulator Envelope
                        values[0] = fm_synth_params->mod_env_atk;
                        values[1] = fm_synth_params->mod_env_dec;
                        values[2] = fm_synth_params->mod_env_sus_level;
                        values[3] = fm_synth_params->mod_env_rel;
                    }
                } else {
                    values[0] = 0;
                    values[1] = 0;
                    values[2] = 0;
                    values[3] = 0;
                }
            } else { // is_sampler
                OpusSamplerParameters *sampler_params =
                    (OpusSamplerParameters *) params->instrument_data;
                values[0] = sampler_params->env_atk;
                values[1] = sampler_params->env_dec;
                values[2] = sampler_params->env_sus_level;
                values[3] = sampler_params->env_rel;
            }

            snprintf(value_str[0], 16, "%d", (int) values[0]);
            snprintf(value_str[1], 16, "%d", (int) values[1]);
            snprintf(value_str[2], 16, "%.1f", values[2]);
            snprintf(value_str[3], 16, "%d", (int) values[3]);

            float start_x = menu_x + 20;
            for (int i = 0; i < 4; i++) {
                C2D_Font current_font = (i == selected_adsr_option) ? font_heavy : font_angular;
                u32      color        = (i == selected_adsr_option) ? CLR_YELLOW : CLR_WHITE;

                C2D_TextBufClear(text_buf);
                snprintf(text, sizeof(text), "%s %s", labels[i], value_str[i]);
                C2D_TextFontParse(&text_obj, current_font, text_buf, text);
                C2D_TextOptimize(&text_obj);

                float text_width, text_height;
                C2D_TextGetDimensions(&text_obj, 0.5f, 0.5f, &text_width, &text_height);
                float text_y = menu_y + (menu_height - text_height) / 2;
                C2D_DrawText(&text_obj, C2D_WithColor, start_x, text_y, 0.0f, 0.5f, 0.5f, color);
                start_x += text_width + 15;
            }
            break;
        }
        case PARAM_TYPE_INT: {
            snprintf(text, sizeof(text), "%d", atoi(param_to_edit->value_string));
            C2D_TextFontParse(&text_obj, font_heavy, text_buf, text);
            C2D_TextOptimize(&text_obj);

            float text_width, text_height;
            C2D_TextGetDimensions(&text_obj, 0.5f, 0.5f, &text_width, &text_height);

            float text_x = menu_x + (menu_width - text_width) / 2;
            float text_y = menu_y + (menu_height - text_height) / 2;

            C2D_DrawText(&text_obj, C2D_WithColor, text_x, text_y, 0.0f, 0.5f, 0.5f, CLR_YELLOW);
            break;
        }
        default: {
            snprintf(text, sizeof(text), "%s", param_to_edit->value_string);
            C2D_TextFontParse(&text_obj, font_heavy, text_buf, text);
            C2D_TextOptimize(&text_obj);

            float text_width, text_height;
            C2D_TextGetDimensions(&text_obj, 0.5f, 0.5f, &text_width, &text_height);

            float text_x = menu_x + (menu_width - text_width) / 2;
            float text_y = menu_y + (menu_height - text_height) / 2;

            C2D_DrawText(&text_obj, C2D_WithColor, text_x, text_y, 0.0f, 0.5f, 0.5f, CLR_YELLOW);
            break;
        }
        }
    }
}
