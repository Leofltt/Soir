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

#include <3ds.h>
#include <3ds/os.h>
#include <citro2d.h>
#include <opusfile.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))
static const char *PATH = "romfs:/samples/bibop.opus";

int main(int argc, char **argv) {
    osSetSpeedupEnable(true);
    gfxInitDefault();
    romfsInit();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    int ret = 0;

    Session session = { VIEW_MAIN };

    PrintConsole bottomScreen;
    consoleInit(GFX_BOTTOM, &bottomScreen);
    consoleSelect(&bottomScreen);

    C3D_RenderTarget *topScreen = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);

    int   active_track = 0;
    Track tracks[2];

    u32 *audioBuffer1                         = NULL;
    PolyBLEPOscillator *osc                   = NULL;
    Envelope *env                             = NULL;
    SubSynth *subsynth                        = NULL;
    SeqStep *sequence1                        = NULL;
    TrackParameters *trackParamsArray1        = NULL;
    SubSynthParameters *subsynthParamsArray   = NULL;
    Sequencer *seq1                           = NULL;
    u32 *audioBuffer2                         = NULL;
    Envelope *env1                            = NULL;
    OpusSampler *sampler                      = NULL;
    SeqStep *sequence2                        = NULL;
    TrackParameters *trackParamsArray2        = NULL;
    OpusSamplerParameters *opusSamplerParamsArray = NULL;
    Sequencer *seq2                           = NULL;
    OggOpusFile *opusFile                     = NULL;

    ndspInit();
    ndspSetOutputMode(NDSP_OUTPUT_STEREO);

    // Note Frequencies
    int notefreq[] = {
        220, 440, 880, 1760, 3520, 7040,
    };

    int pcfreq[] = { 175, 196, 220, 233, 262, 284, 314 };

    int note = 1;
    int cf   = 4;
    int cf2  = 4;
    int wf   = 0;

    // CLOCK //////////////////////////
    MusicalTime mt    = { .bar = 0, .beat = 0, .deltaStep = 0, .steps = 0, .beats_per_bar = 4 };
    Clock       cl    = { .bpm            = 120.0f,
                          .ticks_per_beat = (60.0f / 120.0f),
                          .ticks          = 0,
                          .ticks_per_step = 0,
                          .status         = STOPPED,
                          .barBeats       = &mt };
    Clock      *clock = &cl;
    setBpm(clock, 120.0f);

    // TRACK 1 (SUB_SYNTH) ///////////////////////////////////////////
    audioBuffer1 = (u32 *) linearAlloc(2 * SAMPLESPERBUF * BYTESPERSAMPLE * NCHANNELS);
    if (!audioBuffer1) {
        printf("Failed to allocate audioBuffer1\n");
        ret = 1;
        goto cleanup;
    }
    initializeTrack(&tracks[0], 0, SUB_SYNTH, SAMPLERATE, SAMPLESPERBUF, audioBuffer1);

    osc = (PolyBLEPOscillator *) linearAlloc(sizeof(PolyBLEPOscillator));
    if (!osc) {
        printf("Failed to allocate osc\n");
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
        printf("Failed to allocate env\n");
        ret = 1;
        goto cleanup;
    }
    *env = defaultEnvelopeStruct(SAMPLERATE);
    updateEnvelope(env, 20, 200, 0.6, 50, 300);

    subsynth = (SubSynth *) linearAlloc(sizeof(SubSynth));
    if (!subsynth) {
        printf("Failed to allocate subsynth\n");
        ret = 1;
        goto cleanup;
    }
    *subsynth                 = (SubSynth) { .osc = osc, .env = env };
    tracks[0].instrument_data = subsynth;

    sequence1 = (SeqStep *) linearAlloc(16 * sizeof(SeqStep));
    if (!sequence1) {
        printf("Failed to allocate sequence1\n");
        ret = 1;
        goto cleanup;
    }
    trackParamsArray1 = (TrackParameters *) linearAlloc(16 * sizeof(TrackParameters));
    if (!trackParamsArray1) {
        printf("Failed to allocate trackParamsArray1\n");
        ret = 1;
        goto cleanup;
    }
    subsynthParamsArray = (SubSynthParameters *) linearAlloc(16 * sizeof(SubSynthParameters));
    if (!subsynthParamsArray) {
        printf("Failed to allocate subsynthParamsArray\n");
        ret = 1;
        goto cleanup;
    }
    for (int i = 0; i < 16; i++) {
        subsynthParamsArray[i] = defaultSubSynthParameters();
        trackParamsArray1[i]   = defaultTrackParameters(0, &subsynthParamsArray[i]);
        sequence1[i]           = (SeqStep) { .active = false };
        sequence1[i].data      = &trackParamsArray1[i];
        if (i % 4 == 0 || i == 0) {
            sequence1[i].active = true;
            ((SubSynthParameters *) (sequence1[i].data->instrument_data))->osc_freq =
                midiToHertz(i + 69);
        }
    }
    seq1 = (Sequencer *) linearAlloc(sizeof(Sequencer));
    if (!seq1) {
        printf("Failed to allocate seq1\n");
        ret = 1;
        goto cleanup;
    }
    *seq1 = (Sequencer) { .cur_step = 0, .steps = sequence1, .n_beats = 4, .steps_per_beat = 4 };
    tracks[0].sequencer = seq1;

    // TRACK 2 (OPUS_SAMPLER) ///////////////////////////////////////////
    int error;
    opusFile = op_open_file(PATH, &error);
    if (error != 0 || !opusFile) {
        printf("Failed to open opus file: %d\n", error);
        ret = 1;
        goto cleanup;
    }

    audioBuffer2 = (u32 *) linearAlloc(2 * OPUSSAMPLESPERFBUF * BYTESPERSAMPLE * NCHANNELS);
    if (!audioBuffer2) {
        printf("Failed to allocate audioBuffer2\n");
        ret = 1;
        goto cleanup;
    }
    initializeTrack(&tracks[1], 1, OPUS_SAMPLER, OPUSSAMPLERATE, OPUSSAMPLESPERFBUF, audioBuffer2);

    env1 = (Envelope *) linearAlloc(sizeof(Envelope));
    if (!env1) {
        printf("Failed to allocate env1\n");
        ret = 1;
        goto cleanup;
    }
    *env1 = defaultEnvelopeStruct(OPUSSAMPLERATE);
    updateEnvelope(env1, 100, 300, 0.9, 200, 2000);

    sampler = (OpusSampler *) linearAlloc(sizeof(OpusSampler));
    if (!sampler) {
        printf("Failed to allocate sampler\n");
        ret = 1;
        goto cleanup;
    }
    *sampler = (OpusSampler) { .audiofile       = opusFile,
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
        printf("Failed to allocate sequence2\n");
        ret = 1;
        goto cleanup;
    }
    trackParamsArray2 = (TrackParameters *) linearAlloc(16 * sizeof(TrackParameters));
    if (!trackParamsArray2) {
        printf("Failed to allocate trackParamsArray2\n");
        ret = 1;
        goto cleanup;
    }
    opusSamplerParamsArray =
        (OpusSamplerParameters *) linearAlloc(16 * sizeof(OpusSamplerParameters));
    if (!opusSamplerParamsArray) {
        printf("Failed to allocate opusSamplerParamsArray\n");
        ret = 1;
        goto cleanup;
    }
    for (int i = 0; i < 16; i++) {
        opusSamplerParamsArray[i] = defaultOpusSamplerParameters(opusFile);
        trackParamsArray2[i]      = defaultTrackParameters(1, &opusSamplerParamsArray[i]);
        sequence2[i]              = (SeqStep) { .active = false };
        sequence2[i].data         = &trackParamsArray2[i];
        // if (i % 4 == 2) {

            
        //     sequence2[i].active = true;
        //     ((OpusSamplerParameters *) (sequence2[i].data->instrument_data))->start_position =
        //         (i / 4) * (op_pcm_total(opusFile, -1) / 4);
        // }
    }
    seq2 = (Sequencer *) linearAlloc(sizeof(Sequencer));
    if (!seq2) {
        printf("Failed to allocate seq2\n");
        ret = 1;
        goto cleanup;
    }
    *seq2 = (Sequencer) { .cur_step = 0, .steps = sequence2, .n_beats = 4, .steps_per_beat = 4 };
    tracks[1].sequencer = seq2;

    printf("\x1b[1;1HL: switch selected track\n");
    printf("\x1b[3;16HActive Track: %i\n", active_track);
    printf("\x1b[5;1Hleft/right: change filter type\n");
    printf("\x1b[6;1Hup/down: change filter frequency\n");

    startClock(clock);

    while (aptMainLoop()) {
        hidScanInput();

        u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();

        if (kHeld & KEY_START)
            break;

        if (kDown & KEY_L) {
            active_track = (active_track + 1) % 2;
            printf("\x1b[3;16HActive Track: %i\n", active_track);
        }

        Track *current_track = &tracks[active_track];

        // // Clear previous track-specific info
        // for (int i = 7; i <= 12; i++) {
        //     printf("\x1b[%d;1H                                        ", i);
        // }
        // for (int i = 16; i <= 17; i++) {
        //     printf("\x1b[%d;1H                                        ", i);
        // }

        if (current_track->instrument_type == SUB_SYNTH) {
            SubSynth *current_synth = (SubSynth *) current_track->instrument_data;
            printf("\x1b[7;1HX/B: change Synth frequency\n");
            printf("\x1b[8;1HY: trigger a note\n");
            printf("\x1b[9;1HA: switch Synth waveform\n");

            if (kDown & KEY_Y) {
                triggerEnvelope(current_synth->env);
            }

            if (kDown & KEY_A) {
                wf = (wf + 1) % 4;
                setWaveform(current_synth->osc, wf);
            }

            if (kDown & KEY_B) {
                note = (note > 0) ? note - 1 : ARRAY_SIZE(pcfreq) - 1;
                setOscFrequency(current_synth->osc, pcfreq[note]);
            } else if (kDown & KEY_X) {
                note = (note < ARRAY_SIZE(pcfreq) - 1) ? note + 1 : 0;
                setOscFrequency(current_synth->osc, pcfreq[note]);
            }

            printf("\x1b[11;1Hnote = %i Hz        ", pcfreq[note]);
            printf("\x1b[12;1Hwaveform = %s         ",
                   waveform_names[current_synth->osc->waveform]);

            // FILTER
            if (kDown & KEY_LEFT) {
                current_track->filter.filter_type =
                    (current_track->filter.filter_type > NDSP_BIQUAD_NONE)
                        ? current_track->filter.filter_type - 1
                        : NDSP_BIQUAD_PEAK;
                current_track->filter.update_params = true;
            } else if (kDown & KEY_RIGHT) {
                current_track->filter.filter_type =
                    (current_track->filter.filter_type < NDSP_BIQUAD_PEAK)
                        ? current_track->filter.filter_type + 1
                        : NDSP_BIQUAD_NONE;
                current_track->filter.update_params = true;
            }

            if (kDown & KEY_DOWN) {
                cf                                  = (cf > 0) ? cf - 1 : ARRAY_SIZE(notefreq) - 1;
                current_track->filter.cutoff_freq   = (float) notefreq[cf];
                current_track->filter.update_params = true;
            } else if (kDown & KEY_UP) {
                cf                                  = (cf < ARRAY_SIZE(notefreq) - 1) ? cf + 1 : 0;
                current_track->filter.cutoff_freq   = (float) notefreq[cf];
                current_track->filter.update_params = true;
            }

        } else if (current_track->instrument_type == OPUS_SAMPLER) {
            OpusSampler *current_sampler = (OpusSampler *) current_track->instrument_data;
            printf("\x1b[7;1HY/X/A/B: change Sampler sample start position\n");
            printf("\x1b[8;1HR: change Sampler playback mode\n");

            if (kDown & KEY_R) {
                if (!isLooping(current_sampler)) {
                    current_sampler->seek_requested = true;
                    current_sampler->playback_mode  = LOOP;
                } else {
                    current_sampler->playback_mode = ONE_SHOT;
                }
            }

            if (kDown & KEY_Y) {
                current_sampler->start_position = 0;
                current_sampler->seek_requested = true;
            }
            if (kDown & KEY_X) {
                current_sampler->start_position = op_pcm_total(opusFile, -1) / 4;
                current_sampler->seek_requested = true;
            }
            if (kDown & KEY_A) {
                current_sampler->start_position = op_pcm_total(opusFile, -1) / 2;
                current_sampler->seek_requested = true;
            }
            if (kDown & KEY_B) {
                current_sampler->start_position = (op_pcm_total(opusFile, -1) * 3) / 4;
                current_sampler->seek_requested = true;
            }

            printf("\x1b[11;1Hstart pos = %llu         ", current_sampler->start_position);
            printf("\x1b[12;1Hplayback mode = %s         ",
                   isLooping(current_sampler) ? "LOOP" : "ONE SHOT");

            // FILTER
            if (kDown & KEY_LEFT) {
                current_track->filter.filter_type =
                    (current_track->filter.filter_type > NDSP_BIQUAD_NONE)
                        ? current_track->filter.filter_type - 1
                        : NDSP_BIQUAD_PEAK;
                current_track->filter.update_params = true;
            } else if (kDown & KEY_RIGHT) {
                current_track->filter.filter_type =
                    (current_track->filter.filter_type < NDSP_BIQUAD_PEAK)
                        ? current_track->filter.filter_type + 1
                        : NDSP_BIQUAD_NONE;
                current_track->filter.update_params = true;
            }

            if (kDown & KEY_DOWN) {
                cf2                               = (cf2 > 0) ? cf2 - 1 : ARRAY_SIZE(notefreq) - 1;
                current_track->filter.cutoff_freq = (float) notefreq[cf2];
                current_track->filter.update_params = true;
            } else if (kDown & KEY_UP) {
                cf2                               = (cf2 < ARRAY_SIZE(notefreq) - 1) ? cf2 + 1 : 0;
                current_track->filter.cutoff_freq = (float) notefreq[cf2];
                current_track->filter.update_params = true;
            }
        }

        printf("\x1b[16;1Hfilter = %s         ",
               ndsp_biquad_filter_names[current_track->filter.filter_type]);
        int current_cf = (current_track->instrument_type == SUB_SYNTH) ? cf : cf2;
        printf("\x1b[17;1Hcf = %i Hz          ", notefreq[current_cf]);

        if (current_track->filter.update_params) {
            updateNdspbiquad(current_track->filter);
            current_track->filter.update_params = false;
        }

        // Fill DSP Bufs for each track
        if (tracks[0].waveBuf[tracks[0].fillBlock].status == NDSP_WBUF_DONE) {
            fillSubSynthAudiobuffer(&tracks[0].waveBuf[tracks[0].fillBlock],
                                    tracks[0].waveBuf[tracks[0].fillBlock].nsamples,
                                    (SubSynth *) tracks[0].instrument_data, 1, 0);
            tracks[0].fillBlock = !tracks[0].fillBlock;
        }

        if (tracks[1].waveBuf[tracks[1].fillBlock].status == NDSP_WBUF_DONE) {
            OpusSampler *s = (OpusSampler *) tracks[1].instrument_data;
            if (s->seek_requested) {
                s->seek_requested = false;
                op_pcm_seek(s->audiofile, s->start_position);
            }
            fillSamplerAudiobuffer(&tracks[1].waveBuf[tracks[1].fillBlock],
                                   tracks[1].waveBuf[tracks[1].fillBlock].nsamples, s, 1);
            tracks[1].fillBlock = !tracks[1].fillBlock;
        }

        // CLOCK STUFF
        bool shouldUpdateSeq = updateClock(clock);
        if (shouldUpdateSeq) {
            updateTrack(&tracks[0], clock);
            updateTrack(&tracks[1], clock);
        }

        printf("\x1b[25;1H%d.%d.%d | %s  ", clock->barBeats->bar, clock->barBeats->beat + 1,
               clock->barBeats->deltaStep, clockStatusName[clock->status]);
        if (tracks[1].sequencer) {
            printf("\x1b[28;1H");
            for (int i = 0; i < (tracks[1].sequencer->n_beats * tracks[1].sequencer->steps_per_beat); i++) {
                if (i == tracks[1].sequencer->cur_step) {
                    printf(tracks[1].sequencer->steps[i].active ? "X" : "x");
                } else {
                    printf(tracks[1].sequencer->steps[i].active ? "1" : "0");
                }
            }
        }
        if (tracks[0].sequencer) {
            printf("\x1b[29;1H");
            for (int i = 0; i < (tracks[0].sequencer->n_beats * tracks[0].sequencer->steps_per_beat); i++) {
                if (i == tracks[0].sequencer->cur_step) {
                    printf(tracks[0].sequencer->steps[i].active ? "X" : "x");
                } else {
                    printf(tracks[0].sequencer->steps[i].active ? "1" : "0");
                }
            }
        }

        // top screen rendering stuff
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(topScreen, CLR_BLACK);
        C2D_SceneBegin(topScreen);

        switch (session.main_screen_view) {
        case VIEW_MAIN:
            drawMainView();
            break;
        case VIEW_SETTINGS:
            break;
        case VIEW_ABOUT:
            break;
        default:
            drawMainView();
            break;
        }

        C3D_FrameEnd(0);
    }

cleanup:
    // Cleanup
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
    if (tracks[0].sequencer) {
        cleanupSequencer(tracks[0].sequencer);
        if (tracks[0].sequencer->steps) {
            linearFree(tracks[0].sequencer->steps);
        }
        linearFree(tracks[0].sequencer);
    }
    if (trackParamsArray1)
        linearFree(trackParamsArray1);
    if (subsynthParamsArray)
        linearFree(subsynthParamsArray);

    if (audioBuffer2)
        linearFree(audioBuffer2);
    if (tracks[1].instrument_data) {
        OpusSampler *s = (OpusSampler *) tracks[1].instrument_data;
        if (s->audiofile)
            op_free(s->audiofile);
        if (s->env) {
            linearFree(s->env);
        }
        linearFree(s);
    }
    if (tracks[1].sequencer) {
        cleanupSequencer(tracks[1].sequencer);
        if (tracks[1].sequencer->steps) {
            linearFree(tracks[1].sequencer->steps);
        }
        linearFree(tracks[1].sequencer);
    }
    if (trackParamsArray2)
        linearFree(trackParamsArray2);
    if (opusSamplerParamsArray)
        linearFree(opusSamplerParamsArray);
    ndspExit();
    C2D_Fini();
    C3D_Fini();
    romfsExit();
    gfxExit();
    return ret;
}
