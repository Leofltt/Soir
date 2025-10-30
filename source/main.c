#include "clock.h"
#include "engine_constants.h"
#include "envelope.h"
#include "filters.h"
#include "oscillators.h"
#include "samplers.h"
#include "sequencer.h"
#include "session_controller.h"
#include "synth.h"
#include "track.h"
#include "track_parameters.h"
#include "ui_constants.h"
#include "views.h"
#include "event_queue.h"

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

static const char *PATH  = "romfs:/samples/bibop.opus";
static const char *PATHS = "sdmc:/soir/samples/bibop.opus";

static Track         tracks[N_TRACKS];
static LightLock     clock_lock;
static LightLock     tracks_lock;
static volatile bool should_exit = false;
static EventQueue    g_event_queue;

static Thread clock_thread;
static Thread audio_thread;

void clock_thread_func(void *arg);
void audio_thread_func(void *arg);

// Helper function to process events on the audio thread
static void processTrackEvent(Event *event) {
    if (!event || event->track_id >= N_TRACKS) {
        return;
    }

    LightLock_Lock(&tracks_lock);

    Track *track = &tracks[event->track_id];
    updateTrackParameters(track, &event->base_params); // Changed from event->params

    if (event->instrument_type == SUB_SYNTH) {
        SubSynthParameters *subsynthParams = &event->instrument_specific_params.subsynth_params; // Changed access
        SubSynth           *ss             = (SubSynth *) track->instrument_data;
        if (subsynthParams && ss) {
            setWaveform(ss->osc, subsynthParams->osc_waveform);
            setOscFrequency(ss->osc, subsynthParams->osc_freq);
            updateEnvelope(ss->env, subsynthParams->env_atk, subsynthParams->env_dec,
                           subsynthParams->env_sus_level, subsynthParams->env_rel,
                           subsynthParams->env_dur);
            triggerEnvelope(ss->env);
        }
    } else if (event->instrument_type == OPUS_SAMPLER) {
        OpusSamplerParameters *opusSamplerParams = &event->instrument_specific_params.sampler_params; // Changed access
        OpusSampler           *s                 = (OpusSampler *) track->instrument_data;
        if (opusSamplerParams && s) {
            s->audiofile      = opusSamplerParams->audiofile;
            s->start_position = opusSamplerParams->start_position;
            s->playback_mode  = opusSamplerParams->playback_mode;
            s->seek_requested = true;
            s->finished       = false;
            updateEnvelope(s->env, opusSamplerParams->env_atk, opusSamplerParams->env_dec,
                           opusSamplerParams->env_sus_level, opusSamplerParams->env_rel,
                           opusSamplerParams->env_dur);
            triggerEnvelope(s->env);
        }
    }

    LightLock_Unlock(&tracks_lock);
}

int main(int argc, char **argv) {
    osSetSpeedupEnable(true);
    gfxInitDefault();
    romfsInit();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    initViews();

    int ret = 0;

    Session session = { .main_screen_view = VIEW_MAIN, .touch_screen_view = VIEW_TOUCH_SETTINGS };

    C3D_RenderTarget *topScreen    = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C3D_RenderTarget *bottomScreen = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    int selected_row                = 0;
    int selected_col                = 0;
    int selected_settings_option    = 0;
    int selected_touch_option       = 0;
    int selected_touch_clock_option = 0;
    int selected_sample_row         = 0;
    int selected_sample_col         = 0;
    int selectedQuitOption          = 0;

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
    OpusSampler           *sampler                = NULL;
    SeqStep               *sequence2              = NULL;
    TrackParameters       *trackParamsArray2      = NULL;
    OpusSamplerParameters *opusSamplerParamsArray = NULL;
    OggOpusFile           *opusFile               = NULL;
    Sequencer             *seq2                   = NULL;

    ndspInit();
    ndspSetOutputMode(NDSP_OUTPUT_STEREO);

    int pcfreq[] = { 175, 196, 220, 233, 262, 284, 314 };

    int note = 1;

    // CLOCK //////////////////////////
    MusicalTime mt    = { .bar = 0, .beat = 0, .deltaStep = 0, .steps = 0, .beats_per_bar = 4 };
    Clock       cl    = { .bpm              = 120.0f,
                          .last_tick_time   = 0,
                          .time_accumulator = 0,
                          .ticks_per_step   = 0,
                          .status           = STOPPED,
                          .barBeats         = &mt };
    Clock      *clock = &cl;
    setBpm(clock, 60.0f);

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
    *osc = (PolyBLEPOscillator) { .frequency  = pcfreq[note],
                                  .samplerate = SAMPLERATE,
                                  .waveform   = SINE,
                                  .phase      = 0.,
                                  .phase_inc  = pcfreq[note] * M_TWOPI / SAMPLERATE };

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
    int error;
    opusFile = op_open_file(PATH, &error);
    if (error != 0 || !opusFile) {
        ret = 1;
        goto cleanup;
    }

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

    sampler = (OpusSampler *) linearAlloc(sizeof(OpusSampler));
    if (!sampler) {
        ret = 1;
        goto cleanup;
    }
    *sampler                  = (OpusSampler) { .audiofile       = opusFile,
                                                .start_position  = 0,
                                                .playback_mode   = ONE_SHOT,
                                                .samples_per_buf = OPUSSAMPLESPERFBUF,
                                                .samplerate      = OPUSSAMPLERATE,
                                                .env             = env1,
                                                .seek_requested  = false,
                                                .finished        = true };
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
        opusSamplerParamsArray[i] = defaultOpusSamplerParameters(opusFile);
        trackParamsArray2[i]      = defaultTrackParameters(1, &opusSamplerParamsArray[i]);
        sequence2[i]              = (SeqStep) { .active = false };
        sequence2[i].data         = &trackParamsArray2[i];
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

    s32 main_prio;
    svcGetThreadPriority(&main_prio, CUR_THREAD_HANDLE);

    clock_thread = threadCreate(clock_thread_func, clock, STACK_SIZE, main_prio - 1, -2, true);
    if (clock_thread == NULL) {
        printf("Failed to create clock thread\n");
        ret = 1;
        goto cleanup;
    }

    audio_thread = threadCreate(audio_thread_func, NULL, STACK_SIZE, main_prio - 2, -2, true);
    if (audio_thread == NULL) {
        printf("Failed to create audio thread\n");
        ret = 1;
        goto cleanup;
    }

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
                    }
                }

                if (kDown & KEY_A) {
                    if (selected_row == 0 && selected_col == 0) {
                        LightLock_Lock(&clock_lock);
                        stopClock(clock);
                        LightLock_Unlock(&clock_lock);
                        session.main_screen_view = VIEW_SETTINGS;
                        selected_settings_option = 0;
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
                        }
                        else { // Track row
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
                    }
                    left_timer = now + HOLD_DELAY_INITIAL;
                } else if (kHeld & KEY_LEFT) {
                    if (now >= left_timer) {
                        if (selected_settings_option == 0) { // BPM
                            setBpm(clock, clock->bpm - 1);
                        }
                        left_timer = now + HOLD_DELAY_REPEAT;
                    }
                }
                if (kDown & KEY_RIGHT) {
                    if (selected_settings_option == 0) { // BPM
                        setBpm(clock, clock->bpm + 1);
                    }
                    right_timer = now + HOLD_DELAY_INITIAL;
                } else if (kHeld & KEY_RIGHT) {
                    if (now >= right_timer) {
                        if (selected_settings_option == 0) { // BPM
                            setBpm(clock, clock->bpm + 1);
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
            case VIEW_TOUCH_SETTINGS:
                break;
            case VIEW_TOUCH_CLOCK_SETTINGS:
                break;
            case VIEW_SAMPLE_MANAGER:
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
        case VIEW_ABOUT:
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
            drawSampleManagerView(selected_sample_row, selected_sample_col);
            break;
        default:
            break;
        }

        C3D_FrameEnd(0);
    }

cleanup:
    should_exit = true;

    threadJoin(clock_thread, U64_MAX);
    threadFree(clock_thread);

    threadJoin(audio_thread, U64_MAX);
    threadFree(audio_thread);

    resetTrack(&tracks[0]);
    resetTrack(&tracks[1]);

    if (audioBuffer1)
        linearFree(audioBuffer1);
    if (tracks[0].instrument_data) {
        SubSynth *ss = (SubSynth *) tracks[0].instrument_data;
        if (ss->env) {
            if (ss->env->env_buffer) {
                linearFree(ss->env->env_buffer);
            }
            linearFree(ss->env);
        }
        if (ss->osc) {
            linearFree(ss->osc);
        }
        linearFree(ss);
    }
    if (subsynthParamsArray)
        linearFree(subsynthParamsArray);
    if (trackParamsArray1)
        linearFree(trackParamsArray1);
    if (tracks[0].sequencer) {
        linearFree(tracks[0].sequencer->steps);
        linearFree(tracks[0].sequencer);
    }

    if (audioBuffer2)
        linearFree(audioBuffer2);
    if (tracks[1].instrument_data) {
        OpusSampler *s = (OpusSampler *) tracks[1].instrument_data;
        if (s->audiofile)
            op_free(s->audiofile);
        if (s->env)
            linearFree(s->env);
        linearFree(s);
    }
    if (tracks[1].sequencer) {
        linearFree(tracks[1].sequencer->steps);
        linearFree(tracks[1].sequencer);
    }
    if (trackParamsArray2)
        linearFree(trackParamsArray2);
    if (opusSamplerParamsArray)
        linearFree(opusSamplerParamsArray);

    deinitViews();
    ndspExit();
    C2D_Fini();
    C3D_Fini();
    romfsExit();
    gfxExit();
    return ret;
}

void clock_thread_func(void *arg) {
    Clock *clock = (Clock *) arg;
    while (!should_exit) {
        LightLock_Lock(&clock_lock);
        int ticks_to_process = updateClock(clock);

        if (ticks_to_process > 0) {
            for (int i = 0; i < ticks_to_process; i++) {
                // update musical time for one tick
                int totBeats               = clock->barBeats->steps / STEPS_PER_BEAT;
                clock->barBeats->bar       = totBeats / clock->barBeats->beats_per_bar;
                clock->barBeats->beat      = (totBeats % clock->barBeats->beats_per_bar);
                clock->barBeats->deltaStep = clock->barBeats->steps % STEPS_PER_BEAT;
                clock->barBeats->steps++;

                for (int track_idx = 0; track_idx < N_TRACKS; track_idx++) {
                    Track *track = &tracks[track_idx];
                    if (!track || !track->sequencer || !clock ||
                        track->sequencer->steps_per_beat == 0) {
                        continue;
                    }

                    int clock_steps_per_seq_step = STEPS_PER_BEAT / track->sequencer->steps_per_beat;
                    if (clock_steps_per_seq_step == 0)
                        continue;

                    if ((clock->barBeats->steps - 1) % clock_steps_per_seq_step == 0) {
                        SeqStep step = updateSequencer(track->sequencer);
                        if (step.active && step.data) {
                            Event event = {
                                .type          = TRIGGER_STEP,
                                .track_id      = track_idx,
                                .base_params   = *step.data, // Copy the base TrackParameters
                                .instrument_type = track->instrument_type
                            };

                            // Copy instrument-specific parameters based on type
                            if (track->instrument_type == SUB_SYNTH) {
                                memcpy(&event.instrument_specific_params.subsynth_params,
                                       step.data->instrument_data,
                                       sizeof(SubSynthParameters));
                            } else if (track->instrument_type == OPUS_SAMPLER) {
                                memcpy(&event.instrument_specific_params.sampler_params,
                                       step.data->instrument_data,
                                       sizeof(OpusSamplerParameters));
                            }
                            event_queue_push(&g_event_queue, event);
                        }
                    }
                }
            }
        }
        LightLock_Unlock(&clock_lock);

        svcSleepThread(1000000);
    }
    threadExit(0);
}

void audio_thread_func(void *arg) {
    while (!should_exit) {
        Event event;
        // Process all pending events
        while (event_queue_pop(&g_event_queue, &event)) {
            processTrackEvent(&event);
        }

        for (int i = 0; i < N_TRACKS; i++) {
            // Get the next buffer in the queue for this track
            ndspWaveBuf *waveBuf = &tracks[i].waveBuf[tracks[i].fillBlock];

            // Check if the buffer is 'DONE' (i.e., finished playing)
            // NDSP_WBUF_DONE is defined in 3ds/ndsp/ndsp.h
            if (waveBuf->status == NDSP_WBUF_DONE) {
                // Fill the buffer with new audio data
                if (tracks[i].instrument_type == SUB_SYNTH) {
                    SubSynth *subsynth = (SubSynth *) tracks[i].instrument_data;
                    fillSubSynthAudiobuffer(waveBuf, waveBuf->nsamples, subsynth, 1.0f);
                } else if (tracks[i].instrument_type == OPUS_SAMPLER) {
                    OpusSampler *sampler = (OpusSampler *) tracks[i].instrument_data;
                    if (sampler->seek_requested) {
                        op_pcm_seek(sampler->audiofile, sampler->start_position);
                        sampler->seek_requested = false;
                    }
                    fillSamplerAudiobuffer(waveBuf, waveBuf->nsamples, sampler);
                }
                
                // Add the (now filled) buffer back to the NDSP queue
                ndspChnWaveBufAdd(tracks[i].chan_id, waveBuf);
                
                // Toggle fillBlock to point to the *other* buffer for the next check
                tracks[i].fillBlock = !tracks[i].fillBlock;
            }
        }

        // Sleep for 1ms to prevent busy-waiting and starving other threads
        svcSleepThread(1000000); 
    }
    threadExit(0);
}
