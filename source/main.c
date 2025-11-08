#include "clock.h"
#include "engine_constants.h"
#include "envelope.h"
#include "filters.h"
#include "oscillators.h"
#include "polybleposc.h"
#include "samplers.h"
#include "sample.h"
#include "sequencer.h"
#include "controllers/session_controller.h"
#include "synth.h"
#include "track.h"
#include "track_parameters.h"
#include "ui_constants.h"
#include "ui/ui.h"
#include "event_queue.h"
#include "sample_bank.h"
#include "sample_browser.h"
#include "audio_utils.h"
#include "threads/audio_thread.h"
#include "noise_synth.h"
#include "cleanup_queue.h"

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
static volatile bool         should_exit = false;
static EventQueue            g_event_queue;
SampleBank                   g_sample_bank;
static SampleBrowser         g_sample_browser;
static TrackParameters       g_editing_step_params;
static SubSynthParameters    g_editing_subsynth_params;
static OpusSamplerParameters g_editing_sampler_params;
static FMSynthParameters     g_editing_fm_synth_params;
static NoiseSynthParameters  g_editing_noise_synth_params;

int main(int argc, char **argv) {
    osSetSpeedupEnable(true);
    gfxInitDefault();
    romfsInit();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    initViews();
    sample_cleanup_init(); // <-- RE-ADD
    SampleBankInit(&g_sample_bank);
    SampleBrowserInit(&g_sample_browser);

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
    int  selected_quit_option          = 0;

    u64 up_timer    = 0;
    u64 down_timer  = 0;
    u64 left_timer  = 0;
    u64 right_timer = 0;

    const u64 HOLD_DELAY_INITIAL = 500 * 268125;
    const u64 HOLD_DELAY_REPEAT  = 100 * 268125;

    ScreenFocus screen_focus          = FOCUS_TOP;
    ScreenFocus previous_screen_focus = FOCUS_TOP;

    u32                   *audioBuffer1            = NULL;
    PolyBLEPOscillator    *osc                     = NULL;
    Envelope              *env                     = NULL;
    SubSynth              *subsynth                = NULL;
    SeqStep               *sequence1               = NULL;
    TrackParameters       *trackParamsArray1       = NULL;
    SubSynthParameters    *subsynthParamsArray     = NULL;
    Sequencer             *seq1                    = NULL;
    u32                   *audioBufferFM           = NULL;
    Envelope              *fm_env                  = NULL;
    FMOperator            *fm_op                   = NULL;
    FMSynth               *fm_synth                = NULL;
    SeqStep               *sequenceFM              = NULL;
    TrackParameters       *trackParamsArrayFM      = NULL;
    FMSynthParameters     *fmSynthParamsArray      = NULL;
    Sequencer             *seqFM                   = NULL;
    u32                   *audioBuffer2            = NULL;
    Envelope              *env1                    = NULL;
    Sampler               *sampler                 = NULL;
    SeqStep               *sequence2               = NULL;
    TrackParameters       *trackParamsArray2       = NULL;
    OpusSamplerParameters *opusSamplerParamsArray  = NULL;
    Sequencer             *seq2                    = NULL;
    u32                   *audioBuffer3            = NULL;
    Envelope              *env2                    = NULL;
    Sampler               *sampler2                = NULL;
    SeqStep               *sequence3               = NULL;
    TrackParameters       *trackParamsArray3       = NULL;
    OpusSamplerParameters *opusSamplerParamsArray2 = NULL;
    Sequencer             *seq3                    = NULL;

    u32                  *audioBuffer4          = NULL;
    Envelope             *env3                  = NULL;
    NoiseSynth           *noise_synth           = NULL;
    SeqStep              *sequence4             = NULL;
    TrackParameters      *trackParamsArray4     = NULL;
    NoiseSynthParameters *noiseSynthParamsArray = NULL;
    Sequencer            *seq4                  = NULL;

    s32    main_prio;
    Result rc = svcGetThreadPriority(&main_prio, CUR_THREAD_HANDLE);
    if (R_FAILED(rc)) {
        main_prio = 0x30; // default priority
    }

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
                           .selected_quit_option          = &selected_quit_option,

                           .up_timer    = &up_timer,
                           .down_timer  = &down_timer,
                           .left_timer  = &left_timer,
                           .right_timer = &right_timer,

                           .screen_focus          = &screen_focus,
                           .previous_screen_focus = &previous_screen_focus,

                           .tracks                     = tracks,
                           .clock                      = app_clock,
                           .event_queue                = &g_event_queue,
                           .sample_bank                = &g_sample_bank,
                           .sample_browser             = &g_sample_browser,
                           .editing_step_params        = &g_editing_step_params,
                           .editing_subsynth_params    = &g_editing_subsynth_params,
                           .editing_sampler_params     = &g_editing_sampler_params,
                           .editing_fm_synth_params    = &g_editing_fm_synth_params,
                           .editing_noise_synth_params = &g_editing_noise_synth_params,
                           .clock_lock                 = &clock_lock,

                           .HOLD_DELAY_INITIAL = HOLD_DELAY_INITIAL,
                           .HOLD_DELAY_REPEAT  = HOLD_DELAY_REPEAT };
    setBpm(app_clock, 127.0f);

    // TRACK 0 (SUB_SYNTH) ///////////////////////////////////////////
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
    *osc = (PolyBLEPOscillator) { .frequency   = 220.0f,
                                  .samplerate  = SAMPLERATE,
                                  .waveform    = SINE,
                                  .phase       = 0.,
                                  .pulse_width = 0.5f,
                                  .phase_inc   = 220.0f * M_TWOPI / SAMPLERATE };

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
    *seq1               = (Sequencer) { .cur_step                = 0,
                                        .steps                   = sequence1,
                                        .n_beats                 = 4,
                                        .steps_per_beat          = 4,
                                        .instrument_params_array = subsynthParamsArray,
                                        .track_params_array      = trackParamsArray1 };
    tracks[0].sequencer = seq1;

    // TRACK 1 (FM_SYNTH) ///////////////////////////////////////////
    audioBufferFM = (u32 *) linearAlloc(2 * SAMPLESPERBUF * BYTESPERSAMPLE * NCHANNELS);
    if (!audioBufferFM) {
        ret = 1;
        goto cleanup;
    }
    initializeTrack(&tracks[1], 1, FM_SYNTH, SAMPLERATE, SAMPLESPERBUF, audioBufferFM);

    fm_op = (FMOperator *) linearAlloc(sizeof(FMOperator));
    if (!fm_op) {
        ret = 1;
        goto cleanup;
    }
    fm_op->carrier = (PolyBLEPOscillator *) linearAlloc(sizeof(PolyBLEPOscillator));
    if (!fm_op->carrier) {
        ret = 1;
        goto cleanup;
    }
    fm_op->modulator = (PolyBLEPOscillator *) linearAlloc(sizeof(PolyBLEPOscillator));
    if (!fm_op->modulator) {
        ret = 1;
        goto cleanup;
    }
    fm_op->mod_envelope = (Envelope *) linearAlloc(sizeof(Envelope));
    if (!fm_op->mod_envelope) {
        ret = 1;
        goto cleanup;
    }
    fm_op->mod_index      = 1.0f;
    fm_op->mod_depth      = 100.0f;
    fm_op->base_frequency = 220.0f;

    *fm_op->carrier = (PolyBLEPOscillator) { .frequency   = 220.0f,
                                             .samplerate  = SAMPLERATE,
                                             .waveform    = SINE,
                                             .phase       = 0.0f,
                                             .pulse_width = 0.5f };
    setOscFrequency(fm_op->carrier, fm_op->carrier->frequency);

    *fm_op->modulator = (PolyBLEPOscillator) { .frequency   = 0.0f,
                                               .samplerate  = SAMPLERATE,
                                               .waveform    = SINE,
                                               .phase       = 0.0f,
                                               .pulse_width = 0.5f };
    setOscFrequency(fm_op->modulator, fm_op->modulator->frequency);

    *fm_op->mod_envelope = defaultEnvelopeStruct(SAMPLERATE);
    updateEnvelope(fm_op->mod_envelope, 20, 200, 0.6, 50, 300);

    fm_env = (Envelope *) linearAlloc(sizeof(Envelope));
    if (!fm_env) {
        ret = 1;
        goto cleanup;
    }
    *fm_env = defaultEnvelopeStruct(SAMPLERATE);
    updateEnvelope(fm_env, 20, 200, 0.6, 50, 300);

    fm_synth = (FMSynth *) linearAlloc(sizeof(FMSynth));
    if (!fm_synth) {
        ret = 1;
        goto cleanup;
    }
    *fm_synth                 = (FMSynth) { .carrierEnv = fm_env, .fm_op = fm_op };
    tracks[1].instrument_data = fm_synth;

    sequenceFM = (SeqStep *) linearAlloc(16 * sizeof(SeqStep));
    if (!sequenceFM) {
        ret = 1;
        goto cleanup;
    }
    trackParamsArrayFM = (TrackParameters *) linearAlloc(16 * sizeof(TrackParameters));
    if (!trackParamsArrayFM) {
        ret = 1;
        goto cleanup;
    }
    fmSynthParamsArray = (FMSynthParameters *) linearAlloc(16 * sizeof(FMSynthParameters));
    if (!fmSynthParamsArray) {
        ret = 1;
        goto cleanup;
    }
    for (int i = 0; i < 16; i++) {
        fmSynthParamsArray[i] = defaultFMSynthParameters();
        trackParamsArrayFM[i] = defaultTrackParameters(1, &fmSynthParamsArray[i]);
        sequenceFM[i]         = (SeqStep) { .active = false };
        sequenceFM[i].data    = &trackParamsArrayFM[i];
    }
    seqFM = (Sequencer *) linearAlloc(sizeof(Sequencer));
    if (!seqFM) {
        ret = 1;
        goto cleanup;
    }
    *seqFM              = (Sequencer) { .cur_step                = 0,
                                        .steps                   = sequenceFM,
                                        .n_beats                 = 4,
                                        .steps_per_beat          = 4,
                                        .instrument_params_array = fmSynthParamsArray,
                                        .track_params_array      = trackParamsArrayFM };
    tracks[1].sequencer = seqFM;

    // TRACK 2 (OPUS_SAMPLER) ///////////////////////////////////////////
    audioBuffer2 = (u32 *) linearAlloc(2 * OPUSSAMPLESPERFBUF * BYTESPERSAMPLE * NCHANNELS);
    if (!audioBuffer2) {
        ret = 1;
        goto cleanup;
    }
    initializeTrack(&tracks[2], 2, OPUS_SAMPLER, OPUSSAMPLERATE, OPUSSAMPLESPERFBUF, audioBuffer2);

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
    *sampler = (Sampler) { .sample          = SampleBankGetSample(&g_sample_bank, 0),
                           .start_position  = 0,
                           .playback_mode   = ONE_SHOT,
                           .samples_per_buf = OPUSSAMPLESPERFBUF,
                           .samplerate      = OPUSSAMPLERATE,
                           .env             = env1,
                           .current_frame   = 0,
                           .finished        = true };
    sample_inc_ref(sampler->sample);
    tracks[2].instrument_data = sampler;

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
        trackParamsArray2[i] = defaultTrackParameters(2, &opusSamplerParamsArray[i]);
        sequence2[i]         = (SeqStep) { .active = false };
        sequence2[i].data    = &trackParamsArray2[i];
    }
    seq2 = (Sequencer *) linearAlloc(sizeof(Sequencer));
    if (!seq2) {
        ret = 1;
        goto cleanup;
    }
    *seq2               = (Sequencer) { .cur_step                = 0,
                                        .steps                   = sequence2,
                                        .n_beats                 = 4,
                                        .steps_per_beat          = 4,
                                        .instrument_params_array = opusSamplerParamsArray,
                                        .track_params_array      = trackParamsArray2 };
    tracks[2].sequencer = seq2;

    // TRACK 3 (OPUS_SAMPLER) ///////////////////////////////////////////
    audioBuffer3 = (u32 *) linearAlloc(2 * OPUSSAMPLESPERFBUF * BYTESPERSAMPLE * NCHANNELS);
    if (!audioBuffer3) {
        ret = 1;
        goto cleanup;
    }
    initializeTrack(&tracks[3], 3, OPUS_SAMPLER, OPUSSAMPLERATE, OPUSSAMPLESPERFBUF, audioBuffer3);

    env2 = (Envelope *) linearAlloc(sizeof(Envelope));
    if (!env2) {
        ret = 1;
        goto cleanup;
    }
    *env2 = defaultEnvelopeStruct(OPUSSAMPLERATE);
    updateEnvelope(env2, 100, 300, 0.9, 200, 2000);

    sampler2 = (Sampler *) linearAlloc(sizeof(Sampler));
    if (!sampler2) {
        ret = 1;
        goto cleanup;
    }
    *sampler2 = (Sampler) { .sample          = SampleBankGetSample(&g_sample_bank, 0),
                            .start_position  = 0,
                            .playback_mode   = ONE_SHOT,
                            .samples_per_buf = OPUSSAMPLESPERFBUF,
                            .samplerate      = OPUSSAMPLERATE,
                            .env             = env2,
                            .current_frame   = 0,
                            .finished        = true };
    sample_inc_ref(sampler2->sample);
    tracks[3].instrument_data = sampler2;

    sequence3 = (SeqStep *) linearAlloc(16 * sizeof(SeqStep));
    if (!sequence3) {
        ret = 1;
        goto cleanup;
    }
    trackParamsArray3 = (TrackParameters *) linearAlloc(16 * sizeof(TrackParameters));
    if (!trackParamsArray3) {
        ret = 1;
        goto cleanup;
    }
    opusSamplerParamsArray2 =
        (OpusSamplerParameters *) linearAlloc(16 * sizeof(OpusSamplerParameters));
    if (!opusSamplerParamsArray2) {
        ret = 1;
        goto cleanup;
    }
    for (int i = 0; i < 16; i++) {
        opusSamplerParamsArray2[i]              = defaultOpusSamplerParameters();
        opusSamplerParamsArray2[i].sample_index = 0;
        trackParamsArray3[i] = defaultTrackParameters(3, &opusSamplerParamsArray2[i]);
        sequence3[i]         = (SeqStep) { .active = false };
        sequence3[i].data    = &trackParamsArray3[i];
    }
    seq3 = (Sequencer *) linearAlloc(sizeof(Sequencer));
    if (!seq3) {
        ret = 1;
        goto cleanup;
    }
    *seq3               = (Sequencer) { .cur_step                = 0,
                                        .steps                   = sequence3,
                                        .n_beats                 = 4,
                                        .steps_per_beat          = 4,
                                        .instrument_params_array = opusSamplerParamsArray2,
                                        .track_params_array      = trackParamsArray3 };
    tracks[3].sequencer = seq3;

    // TRACK 4 (NOISE_SYNTH) ///////////////////////////////////////////
    audioBuffer4 = (u32 *) linearAlloc(2 * SAMPLESPERBUF * BYTESPERSAMPLE * NCHANNELS);
    if (!audioBuffer4) {
        ret = 1;
        goto cleanup;
    }
    initializeTrack(&tracks[4], 4, NOISE_SYNTH, SAMPLERATE, SAMPLESPERBUF, audioBuffer4);

    env3 = (Envelope *) linearAlloc(sizeof(Envelope));
    if (!env3) {
        ret = 1;
        goto cleanup;
    }
    *env3 = defaultEnvelopeStruct(SAMPLERATE);
    updateEnvelope(env3, 1, 50, 1.0, 50, 100);

    noise_synth = (NoiseSynth *) linearAlloc(sizeof(NoiseSynth));
    if (!noise_synth) {
        ret = 1;
        goto cleanup;
    }
    *noise_synth = (NoiseSynth) { .env = env3, .lfsr_register = 0x4000 }; // Initial seed
    tracks[4].instrument_data = noise_synth;

    sequence4 = (SeqStep *) linearAlloc(16 * sizeof(SeqStep));
    if (!sequence4) {
        ret = 1;
        goto cleanup;
    }
    trackParamsArray4 = (TrackParameters *) linearAlloc(16 * sizeof(TrackParameters));
    if (!trackParamsArray4) {
        ret = 1;
        goto cleanup;
    }
    noiseSynthParamsArray = (NoiseSynthParameters *) linearAlloc(16 * sizeof(NoiseSynthParameters));
    if (!noiseSynthParamsArray) {
        ret = 1;
        goto cleanup;
    }
    for (int i = 0; i < 16; i++) {
        noiseSynthParamsArray[i] = defaultNoiseSynthParameters();
        trackParamsArray4[i]     = defaultTrackParameters(4, &noiseSynthParamsArray[i]);
        sequence4[i]             = (SeqStep) { .active = false };
        sequence4[i].data        = &trackParamsArray4[i];
    }
    seq4 = (Sequencer *) linearAlloc(sizeof(Sequencer));
    if (!seq4) {
        ret = 1;
        goto cleanup;
    }
    *seq4               = (Sequencer) { .cur_step                = 0,
                                        .steps                   = sequence4,
                                        .n_beats                 = 4,
                                        .steps_per_beat          = 4,
                                        .instrument_params_array = noiseSynthParamsArray,
                                        .track_params_array      = trackParamsArray4 };
    tracks[4].sequencer = seq4;

    LightLock_Init(&clock_lock);
    eventQueueInit(&g_event_queue);

    if (R_FAILED(audioThreadInit(tracks, &g_event_queue, &g_sample_bank, app_clock, &clock_lock,
                                 &should_exit, main_prio))) {
        ret = 1;
        goto cleanup;
    }
    if (R_FAILED(audioThreadStart())) {
        ret = 1;
        goto cleanup;
    }

    const char *quitMenuOptions[]  = { "Quit", "Cancel" };
    const int   numQuitMenuOptions = sizeof(quitMenuOptions) / sizeof(quitMenuOptions[0]);
    bool        should_break_loop  = false;

    while (aptMainLoop()) {
        hidScanInput();

        u64 now   = svcGetSystemTick();
        u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();

        sessionControllerHandleInput(&ctx, kDown, kHeld, now, &should_break_loop);

        if (should_break_loop) {
            break;
        }

        sample_cleanup_process();

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(topScreen, CLR_BLACK);
        C2D_SceneBegin(topScreen);

        drawMainView(tracks, app_clock, selected_row, selected_col, screen_focus);

        switch (session.main_screen_view) {
        case VIEW_MAIN:
            break;
        case VIEW_SETTINGS:
            drawClockSettingsView(app_clock, selected_settings_option);
            break;
        case VIEW_QUIT:
            drawQuitMenu(quitMenuOptions, numQuitMenuOptions, selected_quit_option);
            break;
        case VIEW_STEP_SETTINGS_EDIT:
            drawStepSettingsEditView(&tracks[selected_row - 1], &g_editing_step_params,
                                     selected_step_option, selected_adsr_option, &g_sample_bank);
            break;
        default:
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
                                  &g_sample_browser, screen_focus);
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

    // 1. Signal the audio thread to stop
    should_exit = true;

    // 2. Wait for the audio thread to finish
    audioThreadStopAndJoin();

    // 3. Process any remaining samples queued for cleanup by the audio thread
    //    (This must be done AFTER the audio thread is joined)
    sample_cleanup_process();

    // 4. Shut down NDSP (audio hardware)
    for (int i = 0; i < N_TRACKS; i++) {
        ndspChnWaveBufClear(tracks[i].chan_id);
        // Manually de-allocate all memory for each track
        Track_deinit(&tracks[i]);
    }
    ndspExit();

    // 5. Shut down graphics systems
    // Manually delete the C2D render targets
    C3D_RenderTargetDelete(topScreen);
    C3D_RenderTargetDelete(bottomScreen);

    // Now de-init the views (which frees fonts, etc.)
    deinitViews();

    // Now shut down the libraries
    C2D_Fini();
    C3D_Fini();

    // 6. Shut down services
    romfsExit();
    gfxExit();

    // All linearAlloc'd memory should now be properly freed.

    return ret;
}
