#include "clock.h"
#include "engine_constants.h"
#include "envelope.h"
#include "filters.h"
#include "oscillators.h"
#include "polybleposc.h"
#include "samplers.h"
#include "sample.h"
#include "sequencer.h"
#include "session_controller.h"
#include "synth.h"
#include "track.h"
#include "track_parameters.h"
#include "ui_constants.h"
#include "views.h"
#include "event_queue.h"
#include "sample_bank.h"
#include "sample_browser.h"
#include "audio_utils.h"
#include "threads/audio_thread.h"
#include "threads/clock_timer_thread.h"

#include <3ds.h>
#include <3ds/os.h>
#include <3ds/ndsp/ndsp.h> // Added for ndspChnWaveBufGet
#include <citro2d.h>
#include <opusfile.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <3ds/thread.h>

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))
#define STACK_SIZE (N_TRACKS * 32 * 1024)

static Track                 tracks[N_TRACKS];
static LightLock             clock_lock;
static LightLock             tracks_lock;
static volatile bool         should_exit = false;
static EventQueue            g_event_queue;
static SampleBank            g_sample_bank;
static SampleBrowser         g_sample_browser;
static TrackParameters       g_editing_step_params;
static SubSynthParameters    g_editing_subsynth_params;
static OpusSamplerParameters g_editing_sampler_params;

// Helper function to process events on the audio thread

int main(int argc, char **argv) {
    osSetSpeedupEnable(true);
    gfxInitDefault();
    romfsInit();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    initViews();
    SampleBank_init(&g_sample_bank);
    SampleBrowser_init(&g_sample_browser);

    int ret = 0;

    Session session = { .main_screen_view = VIEW_MAIN, .touch_screen_view = VIEW_TOUCH_SETTINGS };

    C3D_RenderTarget *topScreen    = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C3D_RenderTarget *bottomScreen = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    int  selected_row                  = 0;
    int  selected_col                  = 0;
    int  selected_settings_option      = 0;
    int  selected_touch_option         = 0;
    int  selected_touch_clock_option   = 0;
    int  selected_sample_row           = 0;
    int  selected_sample_col           = 0;
    bool is_selecting_sample           = false;
    int  selected_sample_browser_index = 0;
    int  selected_step_option          = 0;
    int  selected_adsr_option          = 0;
    int  selectedQuitOption            = 0;

    u64 up_timer    = 0;
    u64 down_timer  = 0;
    u64 left_timer  = 0;
    u64 right_timer = 0;

    const u64 HOLD_DELAY_INITIAL = 500 * 268125;
    const u64 HOLD_DELAY_REPEAT  = 100 * 268125;

    ScreenFocus screen_focus          = FOCUS_TOP;
    ScreenFocus previous_screen_focus = FOCUS_TOP;

    u32                   *audioBuffer1           = NULL;
    PolyBLEPOscillator    *osc                    = NULL;
    Envelope              *env                    = NULL;
    SubSynth              *subsynth               = NULL;
    SeqStep               *sequence1              = NULL;
    TrackParameters       *trackParamsArray1      = NULL;
    SubSynthParameters    *subsynthParamsArray    = NULL;
    Sequencer             *seq1                   = NULL;
    u32                   *audioBuffer2           = NULL;
    Envelope              *env1                   = NULL;
    Sampler               *sampler                = NULL;
    SeqStep               *sequence2              = NULL;
    TrackParameters       *trackParamsArray2      = NULL;
    OpusSamplerParameters *opusSamplerParamsArray = NULL;
    Sequencer             *seq2                   = NULL;

    s32 main_prio;
    svcGetThreadPriority(&main_prio, CUR_THREAD_HANDLE);

    ndspInit();
    ndspSetOutputMode(NDSP_OUTPUT_STEREO);

    // CLOCK //////////////////////////
    MusicalTime mt    = { .bar = 0, .beat = 0, .deltaStep = 0, .steps = 0, .beats_per_bar = 4 };
    Clock       cl    = { .bpm              = 120.0f,
                          .last_tick_time   = 0,
                          .time_accumulator = 0,
                          .ticks_per_step   = 0,
                          .status           = STOPPED,
                          .barBeats         = &mt };
    Clock      *clock = &cl;
    setBpm(clock, 127.0f);

    audio_thread_init(tracks, &tracks_lock, &g_event_queue, &g_sample_bank, &should_exit,
                      main_prio);
    clock_timer_threads_init(clock, &clock_lock, &tracks_lock, &g_event_queue, tracks, &should_exit,
                             main_prio);

    // TRACK 1 (SUB_SYNTH) ///////////////////////////////////////////
    audioBuffer1 = (u32 *) linearAlloc(2 * SAMPLESPERBUF * BYTESPERSAMPLE * NCHANNELS);
    if (!audioBuffer1) {
        ret = 1;
        goto cleanup;
    }
    initializeTrack(&tracks[0], 0, SUB_SYNTH, SAMPLERATE, SAMPLESPERBUF, audioBuffer1);

    osc = (PolyBLEPOscillator *) linearAlloc(sizeof(PolyBLEPOscillator));
    if (!osc) {
        ret = 1;
        goto cleanup;
    }
    *osc = (PolyBLEPOscillator) { .frequency  = 220.0f,
                                  .samplerate = SAMPLERATE,
                                  .waveform   = SINE,
                                  .phase      = 0.,
                                  .phase_inc  = 220.0f * M_TWOPI / SAMPLERATE };

    env = (Envelope *) linearAlloc(sizeof(Envelope));
    if (!env) {
        ret = 1;
        goto cleanup;
    }
    *env = defaultEnvelopeStruct(SAMPLERATE);
    updateEnvelope(env, 20, 200, 0.6, 50, 300);

    subsynth = (SubSynth *) linearAlloc(sizeof(SubSynth));
    if (!subsynth) {
        ret = 1;
        goto cleanup;
    }
    *subsynth                 = (SubSynth) { .osc = osc, .env = env };
    tracks[0].instrument_data = subsynth;

    sequence1 = (SeqStep *) linearAlloc(16 * sizeof(SeqStep));
    if (!sequence1) {
        ret = 1;
        goto cleanup;
    }
    trackParamsArray1 = (TrackParameters *) linearAlloc(16 * sizeof(TrackParameters));
    if (!trackParamsArray1) {
        ret = 1;
        goto cleanup;
    }
    subsynthParamsArray = (SubSynthParameters *) linearAlloc(16 * sizeof(SubSynthParameters));
    if (!subsynthParamsArray) {
        ret = 1;
        goto cleanup;
    }
    for (int i = 0; i < 16; i++) {
        subsynthParamsArray[i] = defaultSubSynthParameters();
        trackParamsArray1[i]   = defaultTrackParameters(0, &subsynthParamsArray[i]);
        sequence1[i]           = (SeqStep) { .active = false };
        sequence1[i].data      = &trackParamsArray1[i];
    }
    seq1 = (Sequencer *) linearAlloc(sizeof(Sequencer));
    if (!seq1) {
        ret = 1;
        goto cleanup;
    }
    *seq1 = (Sequencer) { .cur_step = 0, .steps = sequence1, .n_beats = 4, .steps_per_beat = 4 };
    tracks[0].sequencer = seq1;

    // TRACK 2 (OPUS_SAMPLER) ///////////////////////////////////////////
    audioBuffer2 = (u32 *) linearAlloc(2 * OPUSSAMPLESPERFBUF * BYTESPERSAMPLE * NCHANNELS);
    if (!audioBuffer2) {
        ret = 1;
        goto cleanup;
    }
    initializeTrack(&tracks[1], 1, OPUS_SAMPLER, OPUSSAMPLERATE, OPUSSAMPLESPERFBUF, audioBuffer2);

    env1 = (Envelope *) linearAlloc(sizeof(Envelope));
    if (!env1) {
        ret = 1;
        goto cleanup;
    }
    *env1 = defaultEnvelopeStruct(OPUSSAMPLERATE);
    updateEnvelope(env1, 100, 300, 0.9, 200, 2000);

    sampler = (Sampler *) linearAlloc(sizeof(Sampler));
    if (!sampler) {
        ret = 1;
        goto cleanup;
    }
    *sampler = (Sampler) { .sample          = SampleBank_get_sample(&g_sample_bank, 0),
                           .start_position  = 0,
                           .playback_mode   = ONE_SHOT,
                           .samples_per_buf = OPUSSAMPLESPERFBUF,
                           .samplerate      = OPUSSAMPLERATE,
                           .env             = env1,
                           .seek_requested  = false,
                           .finished        = true };
    sample_inc_ref(sampler->sample);
    tracks[1].instrument_data = sampler;

    sequence2 = (SeqStep *) linearAlloc(16 * sizeof(SeqStep));
    if (!sequence2) {
        ret = 1;
        goto cleanup;
    }
    trackParamsArray2 = (TrackParameters *) linearAlloc(16 * sizeof(TrackParameters));
    if (!trackParamsArray2) {
        ret = 1;
        goto cleanup;
    }
    opusSamplerParamsArray =
        (OpusSamplerParameters *) linearAlloc(16 * sizeof(OpusSamplerParameters));
    if (!opusSamplerParamsArray) {
        ret = 1;
        goto cleanup;
    }
    for (int i = 0; i < 16; i++) {
        opusSamplerParamsArray[i]              = defaultOpusSamplerParameters();
        opusSamplerParamsArray[i].sample_index = 0;
        trackParamsArray2[i] = defaultTrackParameters(1, &opusSamplerParamsArray[i]);
        sequence2[i]         = (SeqStep) { .active = false };
        sequence2[i].data    = &trackParamsArray2[i];
    }
    seq2 = (Sequencer *) linearAlloc(sizeof(Sequencer));
    if (!seq2) {
        ret = 1;
        goto cleanup;
    }
    *seq2 = (Sequencer) { .cur_step = 0, .steps = sequence2, .n_beats = 4, .steps_per_beat = 4 };
    tracks[1].sequencer = seq2;

    LightLock_Init(&clock_lock);
    LightLock_Init(&tracks_lock);
    event_queue_init(&g_event_queue);

    clock_timer_threads_start();

    audio_thread_start();

    startClock(clock);

    const char *quitMenuOptions[]  = { "Quit", "Cancel" };
    const int   numQuitMenuOptions = sizeof(quitMenuOptions) / sizeof(quitMenuOptions[0]);
    bool        should_break_loop  = false;

    while (aptMainLoop()) {
        hidScanInput();

        u64 now   = svcGetSystemTick();
        u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();

        if (kDown & KEY_START) {
            previous_screen_focus    = screen_focus;
            screen_focus             = FOCUS_TOP;
            session.main_screen_view = VIEW_QUIT;
            selectedQuitOption       = 0; // Reset selection when opening
            LightLock_Lock(&clock_lock);
            pauseClock(clock);
            LightLock_Unlock(&clock_lock);
        }

        if (session.main_screen_view != VIEW_QUIT) {
            if (kDown & KEY_R) {
                if (screen_focus == FOCUS_TOP) {
                    screen_focus = FOCUS_BOTTOM;
                } else {
                    screen_focus = FOCUS_TOP;
                }
            }
        }

        if (kDown & KEY_L) {
            if (session.touch_screen_view == VIEW_TOUCH_SETTINGS) {
                session.touch_screen_view = VIEW_STEP_SETTINGS;
            } else if (session.touch_screen_view == VIEW_STEP_SETTINGS) {
                session.touch_screen_view = VIEW_TOUCH_SETTINGS;
            }
        }

        if (screen_focus == FOCUS_TOP) {
            switch (session.main_screen_view) {
            case VIEW_MAIN: {
                if (kDown & KEY_UP) {
                    selected_row = (selected_row > 0) ? selected_row - 1 : N_TRACKS;
                    up_timer     = now + HOLD_DELAY_INITIAL;
                } else if (kHeld & KEY_UP) {
                    if (now >= up_timer) {
                        selected_row = (selected_row > 0) ? selected_row - 1 : N_TRACKS;
                        up_timer     = now + HOLD_DELAY_REPEAT;
                    }
                }

                if (kDown & KEY_DOWN) {
                    selected_row = (selected_row < N_TRACKS) ? selected_row + 1 : 0;
                    down_timer   = now + HOLD_DELAY_INITIAL;
                } else if (kHeld & KEY_DOWN) {
                    if (now >= down_timer) {
                        selected_row = (selected_row < N_TRACKS) ? selected_row + 1 : 0;
                        down_timer   = now + HOLD_DELAY_REPEAT;
                    }
                }

                if (kDown & KEY_LEFT) {
                    selected_col = (selected_col > 0) ? selected_col - 1 : 16;
                    left_timer   = now + HOLD_DELAY_INITIAL;
                } else if (kHeld & KEY_LEFT) {
                    if (now >= left_timer) {
                        selected_col = (selected_col > 0) ? selected_col - 1 : 16;
                        left_timer   = now + HOLD_DELAY_REPEAT;
                    }
                }

                if (kDown & KEY_RIGHT) {
                    selected_col = (selected_col < 16) ? selected_col + 1 : 0;
                    right_timer  = now + HOLD_DELAY_INITIAL;
                } else if (kHeld & KEY_RIGHT) {
                    if (now >= right_timer) {
                        selected_col = (selected_col < 16) ? selected_col + 1 : 0;
                        right_timer  = now + HOLD_DELAY_REPEAT;
                    }
                }

                if (kDown & KEY_Y) {
                    if (selected_row == 0 && selected_col == 0) {
                        LightLock_Lock(&clock_lock);
                        if (clock->status == PLAYING) {
                            pauseClock(clock);
                        } else if (clock->status == PAUSED) {
                            resumeClock(clock);
                        }
                        LightLock_Unlock(&clock_lock);
                    } else if (selected_row > 0 && selected_col > 0) {
                        session.touch_screen_view = VIEW_STEP_SETTINGS;
                        screen_focus              = FOCUS_BOTTOM;
                    }
                }

                if (kDown & KEY_A) {
                    if (selected_row == 0 && selected_col == 0) {
                        LightLock_Lock(&clock_lock);
                        stopClock(clock);
                        LightLock_Unlock(&clock_lock);
                        session.main_screen_view = VIEW_SETTINGS;
                        selected_settings_option = 0;
                    } else if (selected_row > 0 && selected_col == 0) {
                        int track_index = selected_row - 1;
                        if (track_index < N_TRACKS) {
                            LightLock_Lock(&tracks_lock);
                            tracks[track_index].is_muted = !tracks[track_index].is_muted;
                            for (int i = 0; i < tracks[track_index].sequencer->n_beats *
                                                    tracks[track_index].sequencer->steps_per_beat;
                                 i++) {
                                tracks[track_index].sequencer->steps[i].data->is_muted =
                                    tracks[track_index].is_muted;
                            }
                            LightLock_Unlock(&tracks_lock);
                        }
                    } else if (selected_col > 0) {
                        int step_index = selected_col - 1;
                        LightLock_Lock(&tracks_lock);
                        if (selected_row == 0) { // Header row
                            for (int i = 0; i < N_TRACKS; i++) {
                                if (tracks[i].sequencer && tracks[i].sequencer->steps) {
                                    tracks[i].sequencer->steps[step_index].active =
                                        !tracks[i].sequencer->steps[step_index].active;
                                }
                            }
                        } else { // Track row
                            int track_index = selected_row - 1;
                            if (track_index < N_TRACKS && tracks[track_index].sequencer &&
                                tracks[track_index].sequencer->steps) {
                                tracks[track_index].sequencer->steps[step_index].active =
                                    !tracks[track_index].sequencer->steps[step_index].active;
                            }
                        }
                        LightLock_Unlock(&tracks_lock);
                    }
                }

                if (kDown & KEY_X) {
                    if (selected_row == 0 && selected_col == 0) {
                        LightLock_Lock(&clock_lock);
                        if (clock->status == PLAYING || clock->status == PAUSED) {
                            stopClock(clock);
                            LightLock_Lock(&tracks_lock);
                            for (int i = 0; i < N_TRACKS; i++) {
                                if (tracks[i].sequencer) {
                                    tracks[i].sequencer->cur_step = 0;
                                }
                            }
                            LightLock_Unlock(&tracks_lock);
                        } else {
                            startClock(clock);
                        }
                        LightLock_Unlock(&clock_lock);
                    }
                }
            } break;
            case VIEW_SETTINGS:
                if (kDown & KEY_UP) {
                    selected_settings_option =
                        (selected_settings_option > 0) ? selected_settings_option - 1 : 2;
                }
                if (kDown & KEY_DOWN) {
                    selected_settings_option =
                        (selected_settings_option < 2) ? selected_settings_option + 1 : 0;
                }
                if (kDown & KEY_B) {
                    session.main_screen_view = VIEW_MAIN;
                }
                if (kDown & KEY_A && selected_settings_option == 2) {
                    session.main_screen_view = VIEW_MAIN;
                }
                if (kDown & KEY_LEFT) {
                    if (selected_settings_option == 0) { // BPM
                        setBpm(clock, clock->bpm - 1);
                    } else if (selected_touch_clock_option == 1) { // Beats per bar
                        setBeatsPerBar(clock, clock->barBeats->beats_per_bar - 1);
                    }
                    left_timer = now + HOLD_DELAY_INITIAL;
                } else if (kHeld & KEY_LEFT) {
                    if (now >= left_timer) {
                        if (selected_settings_option == 0) { // BPM
                            setBpm(clock, clock->bpm - 1);
                        } else if (selected_settings_option == 1) { // Beats per bar
                            setBeatsPerBar(clock, clock->barBeats->beats_per_bar - 1);
                        }
                        left_timer = now + HOLD_DELAY_REPEAT;
                    }
                }
                if (kDown & KEY_RIGHT) {
                    if (selected_settings_option == 0) { // BPM
                        setBpm(clock, clock->bpm + 1);
                    } else if (selected_settings_option == 1) { // Beats per bar
                        setBeatsPerBar(clock, clock->barBeats->beats_per_bar + 1);
                    }
                    right_timer = now + HOLD_DELAY_INITIAL;
                } else if (kHeld & KEY_RIGHT) {
                    if (now >= right_timer) {
                        if (selected_settings_option == 0) { // BPM
                            setBpm(clock, clock->bpm + 1);
                        } else if (selected_settings_option == 1) { // Beats per bar
                            setBeatsPerBar(clock, clock->barBeats->beats_per_bar + 1);
                        }
                        right_timer = now + HOLD_DELAY_REPEAT;
                    }
                }
                break;
            case VIEW_QUIT:
                if (kDown & KEY_UP) {
                    selectedQuitOption =
                        (selectedQuitOption > 0) ? selectedQuitOption - 1 : numQuitMenuOptions - 1;
                }
                if (kDown & KEY_DOWN) {
                    selectedQuitOption =
                        (selectedQuitOption < numQuitMenuOptions - 1) ? selectedQuitOption + 1 : 0;
                }
                if (kDown & KEY_B) {
                    session.main_screen_view = VIEW_MAIN;
                    screen_focus             = previous_screen_focus;
                }
                if (kDown & KEY_A) {
                    if (selectedQuitOption == 0) { // Quit
                        should_break_loop = true;
                    } else { // Cancel
                        session.main_screen_view = VIEW_MAIN;
                        screen_focus             = previous_screen_focus;
                    }
                }
                break;
            case VIEW_ABOUT:
                break;
            case VIEW_STEP_SETTINGS_EDIT: {
                int track_idx = selected_row - 1;
                if (track_idx < 0)
                    break;
                Track *track = &tracks[track_idx];

                if (kDown & KEY_B) { // Cancel
                    session.main_screen_view = VIEW_MAIN;
                    screen_focus             = FOCUS_BOTTOM;
                } else if (kDown & KEY_A) { // Apply
                    if (selected_col == 0) {
                        // Apply to all steps
                        for (int i = 0;
                             i < track->sequencer->n_beats * track->sequencer->steps_per_beat;
                             i++) {
                            SeqStep *seq_step                 = &track->sequencer->steps[i];
                            void    *original_instrument_data = seq_step->data->instrument_data;
                            memcpy(seq_step->data, &g_editing_step_params, sizeof(TrackParameters));
                            seq_step->data->instrument_data = original_instrument_data;

                            if (track->instrument_type == SUB_SYNTH) {
                                memcpy(seq_step->data->instrument_data, &g_editing_subsynth_params,
                                       sizeof(SubSynthParameters));
                            } else if (track->instrument_type == OPUS_SAMPLER) {
                                memcpy(seq_step->data->instrument_data, &g_editing_sampler_params,
                                       sizeof(OpusSamplerParameters));
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
                                       seq_step->data->instrument_data,
                                       sizeof(OpusSamplerParameters));
                            }
                            event_queue_push(&g_event_queue, event);
                        }

                        // Update default parameters for the track
                        void *original_instrument_data = track->default_parameters->instrument_data;
                        memcpy(track->default_parameters, &g_editing_step_params,
                               sizeof(TrackParameters));
                        track->default_parameters->instrument_data = original_instrument_data;

                        if (track->instrument_type == SUB_SYNTH) {
                            memcpy(track->default_parameters->instrument_data,
                                   &g_editing_subsynth_params, sizeof(SubSynthParameters));
                        } else if (track->instrument_type == OPUS_SAMPLER) {
                            memcpy(track->default_parameters->instrument_data,
                                   &g_editing_sampler_params, sizeof(OpusSamplerParameters));
                        }
                    } else {
                        int step_idx = selected_col - 1;
                        if (step_idx >= 0) {
                            SeqStep *seq_step = &track->sequencer->steps[step_idx];

                            void *original_instrument_data = seq_step->data->instrument_data;
                            memcpy(seq_step->data, &g_editing_step_params, sizeof(TrackParameters));
                            seq_step->data->instrument_data = original_instrument_data;

                            if (track->instrument_type == SUB_SYNTH) {
                                memcpy(seq_step->data->instrument_data, &g_editing_subsynth_params,
                                       sizeof(SubSynthParameters));
                            } else if (track->instrument_type == OPUS_SAMPLER) {
                                memcpy(seq_step->data->instrument_data, &g_editing_sampler_params,
                                       sizeof(OpusSamplerParameters));
                            }
                        }
                    }
                    track->filter.update_params = true;
                    session.main_screen_view    = VIEW_MAIN;
                    screen_focus                = FOCUS_BOTTOM;
                } else {
                    bool is_synth           = track->instrument_type == SUB_SYNTH;
                    bool is_sampler         = track->instrument_type == OPUS_SAMPLER;
                    bool is_envelope_option = (is_synth && selected_step_option == 6) ||
                                              (is_sampler && selected_step_option == 7);

                    if (is_envelope_option) {
                        if (kDown & KEY_LEFT) {
                            selected_adsr_option =
                                (selected_adsr_option > 0) ? selected_adsr_option - 1 : 3;
                        }
                        if (kDown & KEY_RIGHT) {
                            selected_adsr_option =
                                (selected_adsr_option < 3) ? selected_adsr_option + 1 : 0;
                        }

                        if (is_synth) {
                            SubSynthParameters *synth_params = &g_editing_subsynth_params;
                            if (selected_adsr_option == 0) { // Attack
                                if (kDown & KEY_UP) {
                                    synth_params->env_atk += 10;
                                    int sum = synth_params->env_atk + synth_params->env_dec +
                                              synth_params->env_rel;
                                    if (sum > synth_params->env_dur) {
                                        synth_params->env_atk =
                                            synth_params->env_dur -
                                            (synth_params->env_dec + synth_params->env_rel);
                                    }
                                }
                                if (kDown & KEY_DOWN)
                                    synth_params->env_atk -= 10;
                                if (synth_params->env_atk < 0)
                                    synth_params->env_atk = 0;
                            } else if (selected_adsr_option == 1) { // Decay
                                if (kDown & KEY_UP) {
                                    synth_params->env_dec += 10;
                                    int sum = synth_params->env_atk + synth_params->env_dec +
                                              synth_params->env_rel;
                                    if (sum > synth_params->env_dur) {
                                        synth_params->env_dec =
                                            synth_params->env_dur -
                                            (synth_params->env_atk + synth_params->env_rel);
                                    }
                                }
                                if (kDown & KEY_DOWN)
                                    synth_params->env_dec -= 10;
                                if (synth_params->env_dec < 0)
                                    synth_params->env_dec = 0;
                            } else if (selected_adsr_option == 2) { // Sustain
                                if (kDown & KEY_UP)
                                    synth_params->env_sus_level += 0.1f;
                                if (kDown & KEY_DOWN)
                                    synth_params->env_sus_level -= 0.1f;
                                if (synth_params->env_sus_level < 0.0f)
                                    synth_params->env_sus_level = 0.0f;
                                if (synth_params->env_sus_level > 1.0f)
                                    synth_params->env_sus_level = 1.0f;
                            } else if (selected_adsr_option == 3) { // Release
                                if (kDown & KEY_UP) {
                                    synth_params->env_rel += 10;
                                    int sum = synth_params->env_atk + synth_params->env_dec +
                                              synth_params->env_rel;
                                    if (sum > synth_params->env_dur) {
                                        synth_params->env_rel =
                                            synth_params->env_dur -
                                            (synth_params->env_atk + synth_params->env_dec);
                                    }
                                }
                                if (kDown & KEY_DOWN)
                                    synth_params->env_rel -= 10;
                                if (synth_params->env_rel < 0)
                                    synth_params->env_rel = 0;
                            }
                        } else { // is_sampler
                            OpusSamplerParameters *sampler_params = &g_editing_sampler_params;
                            if (selected_adsr_option == 0) { // Attack
                                if (kDown & KEY_UP) {
                                    sampler_params->env_atk += 10;
                                    int sum = sampler_params->env_atk + sampler_params->env_dec +
                                              sampler_params->env_rel;
                                    if (sum > sampler_params->env_dur) {
                                        sampler_params->env_atk =
                                            sampler_params->env_dur -
                                            (sampler_params->env_dec + sampler_params->env_rel);
                                    }
                                }
                                if (kDown & KEY_DOWN)
                                    sampler_params->env_atk -= 10;
                                if (sampler_params->env_atk < 0)
                                    sampler_params->env_atk = 0;
                            } else if (selected_adsr_option == 1) { // Decay
                                if (kDown & KEY_UP) {
                                    sampler_params->env_dec += 10;
                                    int sum = sampler_params->env_atk + sampler_params->env_dec +
                                              sampler_params->env_rel;
                                    if (sum > sampler_params->env_dur) {
                                        sampler_params->env_dec =
                                            sampler_params->env_dur -
                                            (sampler_params->env_atk + sampler_params->env_rel);
                                    }
                                }
                                if (kDown & KEY_DOWN)
                                    sampler_params->env_dec -= 10;
                                if (sampler_params->env_dec < 0)
                                    sampler_params->env_dec = 0;
                            } else if (selected_adsr_option == 2) { // Sustain
                                if (kDown & KEY_UP)
                                    sampler_params->env_sus_level += 0.1f;
                                if (kDown & KEY_DOWN)
                                    sampler_params->env_sus_level -= 0.1f;
                                if (sampler_params->env_sus_level < 0.0f)
                                    sampler_params->env_sus_level = 0.0f;
                                if (sampler_params->env_sus_level > 1.0f)
                                    sampler_params->env_sus_level = 1.0f;
                            } else if (selected_adsr_option == 3) { // Release
                                if (kDown & KEY_UP) {
                                    sampler_params->env_rel += 10;
                                    int sum = sampler_params->env_atk + sampler_params->env_dec +
                                              sampler_params->env_rel;
                                    if (sum > sampler_params->env_dur) {
                                        sampler_params->env_rel =
                                            sampler_params->env_dur -
                                            (sampler_params->env_atk + sampler_params->env_dec);
                                    }
                                }
                                if (kDown & KEY_DOWN)
                                    sampler_params->env_rel -= 10;
                                if (sampler_params->env_rel < 0)
                                    sampler_params->env_rel = 0;
                            }
                        }
                    } else if (selected_step_option == 0) { // Volume
                        if (kDown & KEY_DOWN) {
                            g_editing_step_params.volume -= 0.1f;
                            down_timer = now + HOLD_DELAY_INITIAL;
                        } else if (kHeld & KEY_DOWN) {
                            if (now >= down_timer) {
                                g_editing_step_params.volume -= 0.1f;
                                down_timer = now + HOLD_DELAY_REPEAT;
                            }
                        }
                        if (kDown & KEY_UP) {
                            g_editing_step_params.volume += 0.1f;
                            up_timer = now + HOLD_DELAY_INITIAL;
                        } else if (kHeld & KEY_UP) {
                            if (now >= up_timer) {
                                g_editing_step_params.volume += 0.1f;
                                up_timer = now + HOLD_DELAY_REPEAT;
                            }
                        }
                        if (g_editing_step_params.volume < 0.0f)
                            g_editing_step_params.volume = 0.0f;
                        if (g_editing_step_params.volume > 1.0f)
                            g_editing_step_params.volume = 1.0f;
                    } else if (selected_step_option == 1) { // Pan
                        if (kDown & KEY_DOWN) {
                            g_editing_step_params.pan -= 0.1f;
                            down_timer = now + HOLD_DELAY_INITIAL;
                        } else if (kHeld & KEY_DOWN) {
                            if (now >= down_timer) {
                                g_editing_step_params.pan -= 0.1f;
                                down_timer = now + HOLD_DELAY_REPEAT;
                            }
                        }
                        if (kDown & KEY_UP) {
                            g_editing_step_params.pan += 0.1f;
                            up_timer = now + HOLD_DELAY_INITIAL;
                        } else if (kHeld & KEY_UP) {
                            if (now >= up_timer) {
                                g_editing_step_params.pan += 0.1f;
                                up_timer = now + HOLD_DELAY_REPEAT;
                            }
                        }
                        if (g_editing_step_params.pan < -1.0f)
                            g_editing_step_params.pan = -1.0f;
                        if (g_editing_step_params.pan > 1.0f)
                            g_editing_step_params.pan = 1.0f;
                    } else if (selected_step_option == 2) { // Filter CF
                        if (kDown & KEY_UP) {
                            g_editing_step_params.ndsp_filter_cutoff += 100;
                            up_timer = now + HOLD_DELAY_INITIAL;
                        } else if (kHeld & KEY_UP) {
                            if (now >= up_timer) {
                                g_editing_step_params.ndsp_filter_cutoff += 100;
                                up_timer = now + HOLD_DELAY_REPEAT;
                            }
                        }
                        if (kDown & KEY_DOWN) {
                            g_editing_step_params.ndsp_filter_cutoff -= 100;
                            down_timer = now + HOLD_DELAY_INITIAL;
                        } else if (kHeld & KEY_DOWN) {
                            if (now >= down_timer) {
                                g_editing_step_params.ndsp_filter_cutoff -= 100;
                                down_timer = now + HOLD_DELAY_REPEAT;
                            }
                        }
                        if (g_editing_step_params.ndsp_filter_cutoff < 0)
                            g_editing_step_params.ndsp_filter_cutoff = 0;
                        if (g_editing_step_params.ndsp_filter_cutoff > 20000)
                            g_editing_step_params.ndsp_filter_cutoff = 20000;
                    } else if (selected_step_option == 3) { // Filter Type
                        if (kDown & KEY_UP)
                            g_editing_step_params.ndsp_filter_type =
                                (g_editing_step_params.ndsp_filter_type + 1) % 6;
                        if (kDown & KEY_DOWN)
                            g_editing_step_params.ndsp_filter_type =
                                (g_editing_step_params.ndsp_filter_type - 1 + 6) % 6;
                    } else if (is_synth) {
                        SubSynthParameters *synth_params = &g_editing_subsynth_params;
                        if (selected_step_option == 4) { // MIDI Note
                            int midi_note = hertzToMidi(synth_params->osc_freq);
                            if (kDown & KEY_UP)
                                midi_note++;
                            if (kDown & KEY_DOWN)
                                midi_note--;
                            if (midi_note < 0)
                                midi_note = 0;
                            if (midi_note > 127)
                                midi_note = 127;
                            synth_params->osc_freq = midiToHertz(midi_note);
                        } else if (selected_step_option == 5) { // Waveform
                            if (kDown & KEY_UP)
                                synth_params->osc_waveform =
                                    (synth_params->osc_waveform + 1) % WAVEFORM_COUNT;
                            if (kDown & KEY_DOWN)
                                synth_params->osc_waveform =
                                    (synth_params->osc_waveform - 1 + WAVEFORM_COUNT) %
                                    WAVEFORM_COUNT;
                        } else if (selected_step_option == 7) { // Env Dur
                            if (kDown & KEY_UP)
                                synth_params->env_dur += 50;
                            if (kDown & KEY_DOWN)
                                synth_params->env_dur -= 50;
                            if (synth_params->env_dur < 0)
                                synth_params->env_dur = 0;
                        }
                    } else if (is_sampler) {
                        OpusSamplerParameters *sampler_params = &g_editing_sampler_params;
                        if (selected_step_option == 4) { // Sample
                            int count = SampleBank_get_loaded_sample_count(&g_sample_bank);
                            if (count > 0) {
                                if (kDown & KEY_UP)
                                    sampler_params->sample_index =
                                        (sampler_params->sample_index + 1) % count;
                                if (kDown & KEY_DOWN)
                                    sampler_params->sample_index =
                                        (sampler_params->sample_index - 1 + count) % count;
                            }
                        } else if (selected_step_option == 5) { // Playback Mode
                            if (kDown & KEY_UP || kDown & KEY_DOWN)
                                sampler_params->playback_mode =
                                    (sampler_params->playback_mode == ONE_SHOT) ? LOOP : ONE_SHOT;
                        } else if (selected_step_option == 6) { // Start Pos
                            Sample *sample =
                                SampleBank_get_sample(&g_sample_bank, sampler_params->sample_index);
                            if (sample && sample->pcm_length > 0) {
                                float pos =
                                    (float) sampler_params->start_position / sample->pcm_length;
                                if (kDown & KEY_UP)
                                    pos += 0.01f;
                                if (kDown & KEY_DOWN)
                                    pos -= 0.01f;
                                if (pos < 0.0f)
                                    pos = 0.0f;
                                if (pos > 1.0f)
                                    pos = 1.0f;
                                sampler_params->start_position = pos * sample->pcm_length;
                            }
                        } else if (selected_step_option == 8) { // Env Dur
                            if (kDown & KEY_UP)
                                sampler_params->env_dur += 50;
                            if (kDown & KEY_DOWN)
                                sampler_params->env_dur -= 50;
                            if (sampler_params->env_dur < 0)
                                sampler_params->env_dur = 0;
                        }
                    }
                }
            } break;
            }
        } else if (screen_focus == FOCUS_BOTTOM) {
            switch (session.touch_screen_view) {
            case VIEW_TOUCH_SETTINGS:
                if (kDown & KEY_LEFT) {
                    selected_touch_option =
                        (selected_touch_option > 0) ? selected_touch_option - 1 : 1;
                }
                if (kDown & KEY_RIGHT) {
                    selected_touch_option =
                        (selected_touch_option < 1) ? selected_touch_option + 1 : 0;
                }
                if (kDown & KEY_A && selected_touch_option == 0) {
                    session.touch_screen_view   = VIEW_TOUCH_CLOCK_SETTINGS;
                    selected_touch_clock_option = 0;
                } else if (kDown & KEY_A && selected_touch_option == 1) {
                    session.touch_screen_view = VIEW_SAMPLE_MANAGER;
                    selected_sample_row       = 0;
                    selected_sample_col       = 0;
                }
                if (kDown & KEY_Y && selected_touch_option == 0) {
                    LightLock_Lock(&clock_lock);
                    if (clock->status == PLAYING) {
                        pauseClock(clock);
                    } else if (clock->status == PAUSED) {
                        resumeClock(clock);
                    }
                    LightLock_Unlock(&clock_lock);
                }
                if (kDown & KEY_X && selected_touch_option == 0) {
                    LightLock_Lock(&clock_lock);
                    if (clock->status == PLAYING || clock->status == PAUSED) {
                        stopClock(clock);
                        LightLock_Lock(&tracks_lock);
                        for (int i = 0; i < N_TRACKS; i++) {
                            if (tracks[i].sequencer) {
                                tracks[i].sequencer->cur_step = 0;
                            }
                        }
                        LightLock_Unlock(&tracks_lock);
                    } else {
                        startClock(clock);
                    }
                    LightLock_Unlock(&clock_lock);
                }
                break;
            case VIEW_TOUCH_CLOCK_SETTINGS:
                if (kDown & KEY_UP) {
                    selected_touch_clock_option =
                        (selected_touch_clock_option > 0) ? selected_touch_clock_option - 1 : 2;
                }
                if (kDown & KEY_DOWN) {
                    selected_touch_clock_option =
                        (selected_touch_clock_option < 2) ? selected_touch_clock_option + 1 : 0;
                }
                if (kDown & KEY_B) {
                    session.touch_screen_view = VIEW_TOUCH_SETTINGS;
                }
                if (kDown & KEY_A && selected_touch_clock_option == 2) {
                    session.touch_screen_view = VIEW_TOUCH_SETTINGS;
                }
                if (kDown & KEY_LEFT) {
                    if (selected_touch_clock_option == 0) { // BPM
                        setBpm(clock, clock->bpm - 1);
                    } else if (selected_touch_clock_option == 1) { // Beats per bar
                        setBeatsPerBar(clock, clock->barBeats->beats_per_bar - 1);
                    }
                    left_timer = now + HOLD_DELAY_INITIAL;
                } else if (kHeld & KEY_LEFT) {
                    if (now >= left_timer) {
                        if (selected_touch_clock_option == 0) { // BPM
                            setBpm(clock, clock->bpm - 1);
                        } else if (selected_touch_clock_option == 1) { // Beats per bar
                            setBeatsPerBar(clock, clock->barBeats->beats_per_bar - 1);
                        }
                        left_timer = now + HOLD_DELAY_REPEAT;
                    }
                }
                if (kDown & KEY_RIGHT) {
                    if (selected_touch_clock_option == 0) { // BPM
                        setBpm(clock, clock->bpm + 1);
                    } else if (selected_touch_clock_option == 1) { // Beats per bar
                        setBeatsPerBar(clock, clock->barBeats->beats_per_bar + 1);
                    }
                    right_timer = now + HOLD_DELAY_INITIAL;
                } else if (kHeld & KEY_RIGHT) {
                    if (now >= right_timer) {
                        if (selected_touch_clock_option == 0) { // BPM
                            setBpm(clock, clock->bpm + 1);
                        } else if (selected_touch_clock_option == 1) { // Beats per bar
                            setBeatsPerBar(clock, clock->barBeats->beats_per_bar + 1);
                        }
                        right_timer = now + HOLD_DELAY_REPEAT;
                    }
                }
                break;
            case VIEW_SAMPLE_MANAGER:
                if (is_selecting_sample) {
                    if (kDown & KEY_UP) {
                        selected_sample_browser_index =
                            (selected_sample_browser_index > 0)
                                ? selected_sample_browser_index - 1
                                : SampleBrowser_get_sample_count(&g_sample_browser) - 1;
                    }
                    if (kDown & KEY_DOWN) {
                        selected_sample_browser_index =
                            (selected_sample_browser_index <
                             SampleBrowser_get_sample_count(&g_sample_browser) - 1)
                                ? selected_sample_browser_index + 1
                                : 0;
                    }
                    if (kDown & KEY_A) {
                        int         sample_slot = selected_sample_row * 4 + selected_sample_col;
                        const char *path        = SampleBrowser_get_sample_path(
                            &g_sample_browser, selected_sample_browser_index);
                        SampleBank_load_sample(&g_sample_bank, sample_slot, path);
                        is_selecting_sample = false;
                    }
                    if (kDown & KEY_B) {
                        is_selecting_sample = false;
                    }
                } else {
                    if (kDown & KEY_UP) {
                        selected_sample_row =
                            (selected_sample_row > 0) ? selected_sample_row - 1 : 2;
                    }
                    if (kDown & KEY_DOWN) {
                        selected_sample_row =
                            (selected_sample_row < 2) ? selected_sample_row + 1 : 0;
                    }
                    if (kDown & KEY_LEFT) {
                        selected_sample_col =
                            (selected_sample_col > 0) ? selected_sample_col - 1 : 3;
                    }
                    if (kDown & KEY_RIGHT) {
                        selected_sample_col =
                            (selected_sample_col < 3) ? selected_sample_col + 1 : 0;
                    }
                    if (kDown & KEY_A) {
                        is_selecting_sample           = true;
                        selected_sample_browser_index = 0;
                    }
                    if (kDown & KEY_B) {
                        session.touch_screen_view = VIEW_TOUCH_SETTINGS;
                    }
                }
                break;
            case VIEW_STEP_SETTINGS: {
                int track_idx = selected_row - 1;
                if (track_idx < 0)
                    break;
                Track *track      = &tracks[track_idx];
                bool   is_sampler = track->instrument_type == OPUS_SAMPLER;
                int    num_left   = 4;
                int    num_right  = is_sampler ? 5 : 4;

                int current_col = (selected_step_option < num_left) ? 0 : 1;
                int current_row =
                    (current_col == 0) ? selected_step_option : selected_step_option - num_left;

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
                    selected_col = (selected_col % 16) + 1;
                }

                if (kDown & KEY_B) {
                    selected_row = (selected_row % N_TRACKS) + 1;
                }

                selected_step_option = (current_col == 0) ? current_row : current_row + num_left;

                if (kDown & KEY_A) {
                    if (track_idx >= 0) {
                        bool is_sampler = track->instrument_type == OPUS_SAMPLER;
                        int  max_option = is_sampler ? 8 : 7;

                        if (selected_step_option >= 0 && selected_step_option <= max_option) {
                            if (selected_col == 0) {
                                memcpy(&g_editing_step_params, track->default_parameters,
                                       sizeof(TrackParameters));
                                if (track->instrument_type == SUB_SYNTH) {
                                    memcpy(&g_editing_subsynth_params,
                                           track->default_parameters->instrument_data,
                                           sizeof(SubSynthParameters));
                                    g_editing_step_params.instrument_data =
                                        &g_editing_subsynth_params;
                                } else if (track->instrument_type == OPUS_SAMPLER) {
                                    memcpy(&g_editing_sampler_params,
                                           track->default_parameters->instrument_data,
                                           sizeof(OpusSamplerParameters));
                                    g_editing_step_params.instrument_data =
                                        &g_editing_sampler_params;
                                }
                            } else {
                                SeqStep *seq_step = &track->sequencer->steps[selected_col - 1];
                                memcpy(&g_editing_step_params, seq_step->data,
                                       sizeof(TrackParameters));
                                if (track->instrument_type == SUB_SYNTH) {
                                    memcpy(&g_editing_subsynth_params,
                                           seq_step->data->instrument_data,
                                           sizeof(SubSynthParameters));
                                    g_editing_step_params.instrument_data =
                                        &g_editing_subsynth_params;
                                } else if (track->instrument_type == OPUS_SAMPLER) {
                                    memcpy(&g_editing_sampler_params,
                                           seq_step->data->instrument_data,
                                           sizeof(OpusSamplerParameters));
                                    g_editing_step_params.instrument_data =
                                        &g_editing_sampler_params;
                                }
                            }
                            session.main_screen_view = VIEW_STEP_SETTINGS_EDIT;
                            screen_focus             = FOCUS_TOP;
                            selected_adsr_option     = 0; // Reset ADSR selection
                        }
                    }
                }
            } break;
            default:
                break;
            }
        }

        if (should_break_loop) {
            break;
        }

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(topScreen, CLR_BLACK);
        C2D_SceneBegin(topScreen);

        switch (session.main_screen_view) {
        case VIEW_MAIN:
            drawMainView(tracks, clock, selected_row, selected_col, screen_focus);
            break;
        case VIEW_SETTINGS:
            drawMainView(tracks, clock, selected_row, selected_col, screen_focus);
            drawClockSettingsView(clock, selected_settings_option);
            break;
        case VIEW_QUIT:
            drawMainView(tracks, clock, selected_row, selected_col, screen_focus);
            drawQuitMenu(quitMenuOptions, numQuitMenuOptions, selectedQuitOption);
            break;
        case VIEW_STEP_SETTINGS_EDIT:
            drawMainView(tracks, clock, selected_row, selected_col, screen_focus);
            drawStepSettingsEditView(&tracks[selected_row - 1], &g_editing_step_params,
                                     selected_step_option, selected_adsr_option, &g_sample_bank);
            break;
        default:
            drawMainView(tracks, clock, selected_row, selected_col, screen_focus);
            break;
        }

        C2D_TargetClear(bottomScreen, CLR_BLACK);
        C2D_SceneBegin(bottomScreen);

        switch (session.touch_screen_view) {
        case VIEW_TOUCH_SETTINGS:
            drawTouchScreenSettingsView(selected_touch_option, screen_focus);
            break;
        case VIEW_TOUCH_CLOCK_SETTINGS:
            drawTouchClockSettingsView(clock, selected_touch_clock_option);
            break;
        case VIEW_SAMPLE_MANAGER:
            drawSampleManagerView(&g_sample_bank, selected_sample_row, selected_sample_col,
                                  is_selecting_sample, selected_sample_browser_index,
                                  &g_sample_browser);
            break;
        case VIEW_STEP_SETTINGS:
            drawStepSettingsView(&session, tracks, selected_row, selected_col, selected_step_option,
                                 &g_sample_bank, screen_focus);
            break;
        default:
            break;
        }

        C3D_FrameEnd(0);
    }

cleanup:
    should_exit = true;

    clock_timer_threads_stop_and_join();

    audio_thread_stop_and_join();

    SampleBank_deinit(&g_sample_bank);
    deinitViews();
    C2D_Fini();
    C3D_Fini();
    romfsExit();
    gfxExit();

    // Clear any pending wave buffers and then exit NDSP
    for (int i = 0; i < N_TRACKS; i++) {
        ndspChnWaveBufClear(tracks[i].chan_id);
    }
    ndspExit();

    return ret;
}
