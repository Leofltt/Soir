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
    MusicalTime mt        = { .bar = 0, .beat = 0, .deltaStep = 0, .steps = 0, .beats_per_bar = 4 };
    Clock       cl        = { .bpm              = 120.0f,
                              .last_tick_time   = 0,
                              .time_accumulator = 0,
                              .ticks_per_step   = 0,
                              .status           = STOPPED,
                              .barBeats         = &mt };
    Clock      *app_clock = &cl;

    SessionContext ctx = { .session                       = &session,
                           .selected_row                  = &selected_row,
                           .selected_col                  = &selected_col,
                           .selected_settings_option      = &selected_settings_option,
                           .selected_touch_option         = &selected_touch_option,
                           .selected_touch_clock_option   = &selected_touch_clock_option,
                           .selected_sample_row           = &selected_sample_row,
                           .selected_sample_col           = &selected_sample_col,
                           .is_selecting_sample           = &is_selecting_sample,
                           .selected_sample_browser_index = &selected_sample_browser_index,
                           .selected_step_option          = &selected_step_option,
                           .selected_adsr_option          = &selected_adsr_option,
                           .selectedQuitOption            = &selectedQuitOption,

                           .up_timer    = &up_timer,
                           .down_timer  = &down_timer,
                           .left_timer  = &left_timer,
                           .right_timer = &right_timer,

                           .screen_focus          = &screen_focus,
                           .previous_screen_focus = &previous_screen_focus,

                           .tracks                  = tracks,
                           .clock                   = app_clock,
                           .event_queue             = &g_event_queue,
                           .sample_bank             = &g_sample_bank,
                           .sample_browser          = &g_sample_browser,
                           .editing_step_params     = &g_editing_step_params,
                           .editing_subsynth_params = &g_editing_subsynth_params,
                           .editing_sampler_params  = &g_editing_sampler_params,
                           .clock_lock              = &clock_lock,
                           .tracks_lock             = &tracks_lock,

                           .HOLD_DELAY_INITIAL = HOLD_DELAY_INITIAL,
                           .HOLD_DELAY_REPEAT  = HOLD_DELAY_REPEAT };
    setBpm(app_clock, 127.0f);

    audio_thread_init(tracks, &tracks_lock, &g_event_queue, &g_sample_bank, &should_exit,
                      main_prio);
    clock_timer_threads_init(app_clock, &clock_lock, &tracks_lock, &g_event_queue, tracks,
                             &should_exit, main_prio);

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

    startClock(app_clock);

    const char *quitMenuOptions[]  = { "Quit", "Cancel" };
    const int   numQuitMenuOptions = sizeof(quitMenuOptions) / sizeof(quitMenuOptions[0]);
    bool        should_break_loop  = false;

    while (aptMainLoop()) {
        hidScanInput();

        u64 now   = svcGetSystemTick();
        u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();

        session_controller_handle_input(&ctx, kDown, kHeld, now, &should_break_loop);

        if (should_break_loop) {
            break;
        }

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(topScreen, CLR_BLACK);
        C2D_SceneBegin(topScreen);

        switch (session.main_screen_view) {
        case VIEW_MAIN:
            drawMainView(tracks, app_clock, selected_row, selected_col, screen_focus);
            break;
        case VIEW_SETTINGS:
            drawMainView(tracks, app_clock, selected_row, selected_col, screen_focus);
            drawClockSettingsView(app_clock, selected_settings_option);
            break;
        case VIEW_QUIT:
            drawMainView(tracks, app_clock, selected_row, selected_col, screen_focus);
            drawQuitMenu(quitMenuOptions, numQuitMenuOptions, selectedQuitOption);
            break;
        case VIEW_STEP_SETTINGS_EDIT:
            drawMainView(tracks, app_clock, selected_row, selected_col, screen_focus);
            drawStepSettingsEditView(&tracks[selected_row - 1], &g_editing_step_params,
                                     selected_step_option, selected_adsr_option, &g_sample_bank);
            break;
        default:
            drawMainView(tracks, app_clock, selected_row, selected_col, screen_focus);
            break;
        }

        C2D_TargetClear(bottomScreen, CLR_BLACK);
        C2D_SceneBegin(bottomScreen);

        switch (session.touch_screen_view) {
        case VIEW_TOUCH_SETTINGS:
            drawTouchScreenSettingsView(selected_touch_option, screen_focus);
            break;
        case VIEW_TOUCH_CLOCK_SETTINGS:
            drawTouchClockSettingsView(app_clock, selected_touch_clock_option);
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

    for (int i = 0; i < N_TRACKS; i++) {
        ndspChnWaveBufClear(tracks[i].chan_id);
    }
    ndspExit();

    for (int i = 0; i < N_TRACKS; i++) {
        Track_deinit(&tracks[i]);
    }

    SampleBank_deinit(&g_sample_bank);
    deinitViews();
    C2D_Fini();
    C3D_Fini();
    romfsExit();
    gfxExit();

    return ret;
}
