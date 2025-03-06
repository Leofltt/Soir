#include <3ds.h>
#include <opusfile.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "audio_utils.h"
#include "envelope.h"
#include "filters.h"
#include "oscillators.h"
#include "samplers.h"
#include "synth.h"

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))
// static const char *ROMFS_PATH = "romfs:/";
static const char* PATH = "romfs:/samples/bibop.opus";  // Path to Opus file to play

// static const int THREAD_AFFINITY = -1;           // Execute thread on any core
// static const int THREAD_STACK_SZ = 32 * 1024;    // 32kB stack for audio thread

// ------------------------------------------------------------

int main(int argc, char** argv) {
    gfxInitDefault();

    PrintConsole topScreen;
    consoleInit(GFX_TOP, &topScreen);
    consoleSelect(&topScreen);

    // Enable N3DS 804MHz operation, where available
    osSetSpeedupEnable(true);

    // u32 kDownOld = 0, kHeldOld = 0, kUpOld = 0; //In these variables there will be information
    // about keys detected in the previous frame

    int active_track = 0;

    u32* audioBuffer = (u32*)linearAlloc(SAMPLESPERBUF * BYTESPERSAMPLE * NCHANNELS);

    u32* audioBuffer2 = (u32*)linearAlloc(OPUSSAMPLESPERFBUF * BYTESPERSAMPLE * NCHANNELS);

    bool fillBlock = false;
    bool fillBlock2 = false;

    romfsInit();
    ndspInit();
    ndspSetOutputMode(NDSP_OUTPUT_STEREO);

    // Note Frequencies
    int notefreq[] = {
        220, 440, 880, 1760, 3520, 7040,
    };

    int pcfreq[] = {175, 196, 220, 233, 262, 284, 314};

    int note = 1;
    int cf = 4;
    int cf2 = 4;
    int wf = 0;

    bool s_seekRequested = false;

    size_t stream_offset = 0;
    size_t stream_offset2 = 0;

    // TRACK 1 ///////////////////////////////////////////
    ndspChnReset(0);
    ndspChnSetInterp(0, NDSP_INTERP_LINEAR);
    ndspChnSetRate(0, SAMPLERATE);
    ndspChnSetFormat(0, NDSP_FORMAT_STEREO_PCM16);

    float mix[12];
    memset(mix, 0, sizeof(mix));
    mix[0] = 1.0;
    mix[1] = 1.0;

    ndspChnSetMix(0, mix);

    NdspBiquad biquadFilter = {.cutoff_freq = (float)notefreq[cf],
                               .filter_type = NDSP_BIQUAD_NONE,
                               .update_params = false,
                               .id = 0};
    NdspBiquad* filter = &biquadFilter;

    PolyBLEPOscillator pbOscillator = {.frequency = pcfreq[note],
                                       .samplerate = SAMPLERATE,
                                       .waveform = SINE,
                                       .phase = 0.,
                                       .phase_inc = pcfreq[note] * M_TWOPI / SAMPLERATE};
    PolyBLEPOscillator* osc = &pbOscillator;

    Envelope ampEnvelope = {.atk = 10,
                            .dec = 10,
                            .rel = 10,
                            .dur = 40,
                            .sus_level = 0.6,
                            .sus_time = 10,
                            .gate = ENV_OFF,
                            .env_pos = 0,
                            .sr = SAMPLERATE};
    Envelope* env = &ampEnvelope;
    updateEnvelope(env, 150, 200, 0.4, 150, 650);

    SubSynth subs = {.osc = osc, .env = env};
    SubSynth* subsynth = &subs;

    ndspWaveBuf waveBuf[2];
    memset(waveBuf, 0, sizeof(waveBuf));
    waveBuf[0].data_vaddr = &audioBuffer[0];
    waveBuf[0].nsamples = SAMPLESPERBUF;
    waveBuf[1].data_vaddr = &audioBuffer[SAMPLESPERBUF];
    waveBuf[1].nsamples = SAMPLESPERBUF;

    fillBufferWithZeros(audioBuffer, SAMPLESPERBUF * NCHANNELS);

    ndspChnWaveBufAdd(0, &waveBuf[0]);
    ndspChnWaveBufAdd(0, &waveBuf[1]);
    ////////////////////////////////////////

    // TRACK 2 ///////////////////////////////////////////
    int error = 0;
    OggOpusFile* opusFile = op_open_file(PATH, &error);

    OpusSampler opSampler = {.audiofile = opusFile,
                             .start_position = 0,
                             .playback_mode = LOOP,
                             .samples_per_buf = OPUSSAMPLESPERFBUF,
                             .samplerate = OPUSSAMPLERATE};

    OpusSampler* sampler = &opSampler;

    ndspChnReset(1);
    ndspChnSetInterp(1, NDSP_INTERP_LINEAR);
    ndspChnSetRate(1, OPUSSAMPLERATE);
    ndspChnSetFormat(1, NDSP_FORMAT_STEREO_PCM16);

    float mix2[12];
    memset(mix2, 0, sizeof(mix2));
    mix2[0] = 1.0;
    mix2[1] = 1.0;

    ndspChnSetMix(1, mix2);

    NdspBiquad biquadFilter2 = {.cutoff_freq = (float)notefreq[cf2],
                                .filter_type = NDSP_BIQUAD_NONE,
                                .update_params = false,
                                .id = 1};
    NdspBiquad* filter2 = &biquadFilter2;

    // We set up two wave buffers and alternate between the two,

    ndspWaveBuf waveBuf2[2];
    memset(waveBuf2, 0, sizeof(waveBuf2));
    waveBuf2[0].data_vaddr = &audioBuffer2[0];
    waveBuf2[0].nsamples = OPUSSAMPLESPERFBUF;
    waveBuf2[1].data_vaddr = &audioBuffer2[OPUSSAMPLESPERFBUF];
    waveBuf2[1].nsamples = OPUSSAMPLESPERFBUF;

    fillBufferWithZeros(audioBuffer2, OPUSSAMPLESPERFBUF * NCHANNELS);

    ndspChnWaveBufAdd(1, &waveBuf2[0]);
    ndspChnWaveBufAdd(1, &waveBuf2[1]);
    ////////////////////////////////////////

    stream_offset += SAMPLESPERBUF;
    stream_offset2 += OPUSSAMPLESPERFBUF;

    printf("\x1b[1;1HL: switch selected track\n");
    printf("\x1b[3;16HActive Track: %i\n", active_track);

    printf("\x1b[5;1Hleft/right: change filter type\n");
    printf("\x1b[6;1Hup/down: change filter frequency\n");

    if (active_track == 0) {
        printf("\x1b[7;1HX/B: change Synth frequency\n");
        printf("\x1b[8;1HY: trigger a note\n");
        printf("\x1b[9;1HA: switch Synth waveform\n");
        printf("\x1b[11;1Hnote = %i Hz        ", pcfreq[note]);
        printf("\x1b[12;1Hwaveform = %s        ", waveform_names[subsynth->osc->waveform]);
    } else if (active_track == 1) {
        printf("\x1b[7;1HY/X/A/B: change Sampler sample start position\n");
        printf("\x1b[8;1HR: change Sampler playback mode\n");
        printf("\x1b[11;1Hstart pos = %llu         ", sampler->start_position);
        printf("\x1b[12;1Hplayback mode = %s         ", isLooping(sampler) ? "LOOP" : "ONE SHOT");
        printf("\x1b[16;1Hfilter = %s         ", ndsp_biquad_filter_names[filter2->filter_type]);
        printf("\x1b[17;1Hcf = %i Hz          ", notefreq[cf2]);

    } else {
        printf("ERROR: Invalid active track\n");
    }

    printf("\x1b[30;16HSTART: exit.");

    while (aptMainLoop()) {
        gfxSwapBuffers();
        gfxFlushBuffers();
        gspWaitForVBlank();
        hidScanInput();

        u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();

        ///////////////////////// QUIT w/ Start ////////////////////////

        if (kHeld & KEY_START) break;

        //////////////////////// CONTROLS ////////////////////////

        if (kDown & KEY_L) {
            active_track == 1 ? active_track-- : active_track++;
            printf("\x1b[3;16HActive Track: %i\n", active_track);
        }

        if (active_track == 0) {
            printf("\x1b[7;1HX/B: change Synth frequency\n");
            printf("\x1b[8;1HY: trigger a note\n");
            printf("\x1b[9;1HA: switch Synth waveform\n");

            if (kDown & KEY_Y) {
                // trigger a note
                triggerEnvelope(subsynth->env);
            }

            if (kDown & KEY_A) {
                wf++;
                if (wf >= 4) {
                    wf = 0;
                }
                setWaveform(subsynth->osc, wf);
            }
            printf("\x1b[12;1Hwaveform = %s         ", waveform_names[subsynth->osc->waveform]);

            // OSC NOTE
            if (kDown & KEY_B) {
                note--;
                if (note < 0) {
                    note = ARRAY_SIZE(pcfreq) - 1;
                }
                setOscFrequency(subsynth->osc, pcfreq[note]);
            } else if (kDown & KEY_X) {
                note++;
                if (note >= ARRAY_SIZE(pcfreq)) {
                    note = 0;
                }
                setOscFrequency(subsynth->osc, pcfreq[note]);
            }

            printf("\x1b[11;1Hnote = %i Hz        ", pcfreq[note]);

            // FILTER TYPE
            if (kDown & KEY_LEFT) {
                if (filter->filter_type == NDSP_BIQUAD_NONE) {
                    filter->filter_type = NDSP_BIQUAD_PEAK;
                } else {
                    filter->filter_type--;
                }
                filter->update_params = true;
            } else if (kDown & KEY_RIGHT) {
                if (filter->filter_type == NDSP_BIQUAD_PEAK) {
                    filter->filter_type = NDSP_BIQUAD_NONE;
                } else {
                    filter->filter_type++;
                }
                filter->update_params = true;
            }

            // FILTER FREQ
            if (kDown & KEY_DOWN) {
                cf--;
                if (cf < 0) {
                    cf = ARRAY_SIZE(notefreq) - 1;
                }
                filter->cutoff_freq = (float)notefreq[cf];
                filter->update_params = true;
            } else if (kDown & KEY_UP) {
                cf++;
                if (cf >= ARRAY_SIZE(notefreq)) {
                    cf = 0;
                }
                filter->cutoff_freq = (float)notefreq[cf];
                filter->update_params = true;
            }

            printf("\x1b[16;1Hfilter = %s         ", ndsp_biquad_filter_names[filter->filter_type]);
            printf("\x1b[17;1Hcf = %i Hz          ", notefreq[cf]);

            if (filter->update_params) {
                update_ndspbiquad(*filter);
                filter->update_params = false;
            }
        }

        else if (active_track == 1) {
            printf("\x1b[7;1HY/X/A/B: change Sampler sample start position\n");
            printf("\x1b[8;1HR: change Sampler playback mode\n");

            if (kDown & KEY_R) {
                if (!isLooping(sampler)) {
                    // Restart playback if looping was disabled and playback stopped
                    s_seekRequested = true;
                    sampler->playback_mode = LOOP;
                } else if (isLooping(sampler)) {
                    sampler->playback_mode = ONE_SHOT;
                }
            }
            printf("\x1b[12;1Hplayback mode = %s         ",
                   isLooping(sampler) ? "LOOP" : "ONE SHOT");

            if (kDown & KEY_Y) {
                sampler->start_position = 0;
                s_seekRequested = true;
            }

            if (kDown & KEY_X) {
                sampler->start_position = op_pcm_total(opusFile, -1) / 4;
                s_seekRequested = true;
            }

            if (kDown & KEY_A) {
                sampler->start_position =
                    op_pcm_total(opusFile, -1) / 2;  // Get total samples and divide by 2
                s_seekRequested = true;
            }

            if (kDown & KEY_B) {
                sampler->start_position = (op_pcm_total(opusFile, -1) * 3) / 4;
                s_seekRequested = true;
            }

            printf("\x1b[11;1Hstart pos = %llu         ", sampler->start_position);

            // FILTER TYPE
            if (kDown & KEY_LEFT) {
                if (filter2->filter_type == NDSP_BIQUAD_NONE) {
                    filter2->filter_type = NDSP_BIQUAD_PEAK;
                } else {
                    filter2->filter_type--;
                }
                filter2->update_params = true;
            } else if (kDown & KEY_RIGHT) {
                if (filter2->filter_type == NDSP_BIQUAD_PEAK) {
                    filter2->filter_type = NDSP_BIQUAD_NONE;
                } else {
                    filter2->filter_type++;
                }
                filter2->update_params = true;
            }

            // FILTER FREQ
            if (kDown & KEY_DOWN) {
                cf2--;
                if (cf2 < 0) {
                    cf2 = ARRAY_SIZE(notefreq) - 1;
                }
                filter2->cutoff_freq = (float)notefreq[cf2];
                filter2->update_params = true;
            } else if (kDown & KEY_UP) {
                cf2++;
                if (cf2 >= ARRAY_SIZE(notefreq)) {
                    cf2 = 0;
                }
                filter2->cutoff_freq = (float)notefreq[cf2];
                filter2->update_params = true;
            }

            printf("\x1b[16;1Hfilter = %s         ",
                   ndsp_biquad_filter_names[filter2->filter_type]);
            printf("\x1b[17;1Hcf = %i Hz          ", notefreq[cf2]);

            if (filter2->update_params) {
                update_ndspbiquad(*filter2);
                filter2->update_params = false;
            }
        }

        /////////////////////////////////////////////////////////////////

        if (waveBuf[fillBlock].status == NDSP_WBUF_DONE) {
            fillSubSynthAudiobuffer(&waveBuf[fillBlock], waveBuf[fillBlock].nsamples, subsynth, 1,
                                    0);
            stream_offset += waveBuf[fillBlock].nsamples;
            fillBlock = !fillBlock;
        }

        if (waveBuf2[fillBlock2].status == NDSP_WBUF_DONE) {
            if (s_seekRequested) {
                s_seekRequested = false;
                op_pcm_seek(opusFile, sampler->start_position);
            }
            fillSamplerAudiobuffer(&waveBuf2[fillBlock2], waveBuf2[fillBlock2].nsamples, sampler,
                                   1);
            stream_offset2 += waveBuf2[fillBlock2].nsamples;
            fillBlock2 = !fillBlock2;
        }
    }

    //////// TRACK 1 //////////////////
    ndspChnReset(0);
    linearFree(audioBuffer);
    linearFree((subsynth->env)->env_buffer);
    ////////////////////////////////////

    //////// TRACK 2 //////////////////
    ndspChnReset(1);
    linearFree(audioBuffer2);
    op_free(opusFile);
    /////////////////////////////

    ndspExit();

    romfsExit();
    gfxExit();
    return 0;
}
