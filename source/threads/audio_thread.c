#include "threads/audio_thread.h"
#include "audio_utils.h"
#include "synth.h"
#include "samplers.h"
#include "envelope.h"
#include "engine_constants.h"
#include <3ds/ndsp/ndsp.h> // Added for ndspChnWaveBufGet
#include <stdio.h>
#include <string.h>

// Static global variables for this module
static Thread     s_audio_thread;
static LightEvent s_audio_event;

// Pointers to shared state from main
static Track         *s_tracks_ptr       = NULL;
static LightLock     *s_tracks_lock_ptr  = NULL;
static EventQueue    *s_event_queue_ptr  = NULL;
static SampleBank    *s_sample_bank_ptr  = NULL;
static volatile bool *s_should_exit_ptr  = NULL;
static s32            s_main_thread_prio = 0;

static void audio_callback(void *data) {
    LightEvent_Signal(&s_audio_event);
}

// Helper function to process events on the audio thread
static void processTrackEvent(Event *event) {
    if (!event || event->track_id >= N_TRACKS) {
        return;
    }

    LightLock_Lock(s_tracks_lock_ptr);

    Track *track = &s_tracks_ptr[event->track_id];
    updateTrackParameters(track, &event->base_params);

    if (event->instrument_type == SUB_SYNTH) {
        SubSynthParameters *subsynthParams = &event->instrument_specific_params.subsynth_params;
        SubSynth           *ss             = (SubSynth *) track->instrument_data;
        if (subsynthParams && ss) {
            setWaveform(ss->osc, subsynthParams->osc_waveform);
            setOscFrequency(ss->osc, subsynthParams->osc_freq);
            updateEnvelope(ss->env, subsynthParams->env_atk, subsynthParams->env_dec,
                           subsynthParams->env_sus_level, subsynthParams->env_rel,
                           subsynthParams->env_dur);
            if (event->type == TRIGGER_STEP) {
                triggerEnvelope(ss->env);
            }
        }
    } else if (event->instrument_type == OPUS_SAMPLER) {
        OpusSamplerParameters *opusSamplerParams =
            &event->instrument_specific_params.sampler_params;
        Sampler *s = (Sampler *) track->instrument_data;
        if (opusSamplerParams && s) {
            Sample *new_sample =
                SampleBankGetSample(s_sample_bank_ptr, opusSamplerParams->sample_index);
            if (new_sample != s->sample) {
                sample_inc_ref(new_sample);
                sample_dec_ref(s->sample);
                s->sample = new_sample;
            }

            s->start_position = opusSamplerParams->start_position;
            s->playback_mode  = opusSamplerParams->playback_mode;
            s->seek_requested = true;
            s->finished       = false;
            updateEnvelope(s->env, opusSamplerParams->env_atk, opusSamplerParams->env_dec,
                           opusSamplerParams->env_sus_level, opusSamplerParams->env_rel,
                           opusSamplerParams->env_dur);
            if (event->type == TRIGGER_STEP) {
                triggerEnvelope(s->env);
            }
        }
    } else if (event->instrument_type == FM_SYNTH) {
        FMSynthParameters *fmSynthParams = &event->instrument_specific_params.fm_synth_params;
        FMSynth           *fs            = (FMSynth *) track->instrument_data;
        if (fmSynthParams && fs) {
            FMOpSetCarrierFrequency(fs->fm_op, fmSynthParams->carrier_freq);
            FMOpSetModRatio(fs->fm_op, fmSynthParams->mod_freq_ratio);
            FMOpSetModIndex(fs->fm_op, fmSynthParams->mod_index);
            FMOpSetModDepth(fs->fm_op, fmSynthParams->mod_depth);
            updateEnvelope(fs->carrierEnv, fmSynthParams->carrier_env_atk,
                           fmSynthParams->carrier_env_dec, fmSynthParams->carrier_env_sus_level,
                           fmSynthParams->carrier_env_rel, fmSynthParams->env_dur);
            updateEnvelope(fs->fm_op->mod_envelope, fmSynthParams->mod_env_atk,
                           fmSynthParams->mod_env_dec, fmSynthParams->mod_env_sus_level,
                           fmSynthParams->mod_env_rel, fmSynthParams->env_dur);
            if (event->type == TRIGGER_STEP) {
                triggerEnvelope(fs->carrierEnv);
                triggerEnvelope(fs->fm_op->mod_envelope);
            }
        }
    }

    LightLock_Unlock(s_tracks_lock_ptr);
}

static void audio_thread_entry(void *arg) {
    while (!*s_should_exit_ptr) {
        Event event;
        while (eventQueuePop(s_event_queue_ptr, &event)) {
            processTrackEvent(&event);
        }

        for (int i = 0; i < N_TRACKS; i++) {
            if (s_tracks_ptr[i].filter.update_params) {
                updateNdspbiquad(s_tracks_ptr[i].filter);
                s_tracks_ptr[i].filter.update_params = false;
            }

            ndspWaveBuf *waveBuf = &s_tracks_ptr[i].waveBuf[s_tracks_ptr[i].fillBlock];

            if (waveBuf->status == NDSP_WBUF_DONE) {
                if (s_tracks_ptr[i].instrument_type == SUB_SYNTH) {
                    SubSynth *subsynth = (SubSynth *) s_tracks_ptr[i].instrument_data;
                    fillSubSynthAudiobuffer(waveBuf, waveBuf->nsamples, subsynth);
                } else if (s_tracks_ptr[i].instrument_type == OPUS_SAMPLER) {
                    Sampler *sampler = (Sampler *) s_tracks_ptr[i].instrument_data;
                    if (sampler->seek_requested) {
                        op_pcm_seek(sampler->sample->opusFile, sampler->start_position);
                        sampler->seek_requested = false;
                    }
                    fillSamplerAudioBuffer(waveBuf, waveBuf->nsamples, sampler);
                } else if (s_tracks_ptr[i].instrument_type == FM_SYNTH) {
                    FMSynth *fm_synth = (FMSynth *) s_tracks_ptr[i].instrument_data;
                    fillFMSynthAudiobuffer(waveBuf, waveBuf->nsamples, fm_synth);
                }

                ndspChnWaveBufAdd(s_tracks_ptr[i].chan_id, waveBuf);

                s_tracks_ptr[i].fillBlock = !s_tracks_ptr[i].fillBlock;
            }
        }
        LightEvent_Wait(&s_audio_event);
        if (*s_should_exit_ptr) {
            break;
        }
    }
}

void audioThreadInit(Track *tracks_ptr, LightLock *tracks_lock_ptr, EventQueue *event_queue_ptr,
                     SampleBank *sample_bank_ptr, volatile bool *should_exit_ptr,
                     s32 main_thread_prio) {
    s_tracks_ptr       = tracks_ptr;
    s_tracks_lock_ptr  = tracks_lock_ptr;
    s_event_queue_ptr  = event_queue_ptr;
    s_sample_bank_ptr  = sample_bank_ptr;
    s_should_exit_ptr  = should_exit_ptr;
    s_main_thread_prio = main_thread_prio;
    LightEvent_Init(&s_audio_event, RESET_ONESHOT);
    ndspSetCallback(audio_callback, NULL);
}

void audioThreadStart() {
    s_audio_thread =
        threadCreate(audio_thread_entry, NULL, STACK_SIZE, s_main_thread_prio - 2, -2, true);
    if (s_audio_thread == NULL) {
        *s_should_exit_ptr = true;
    }
}

void audioThreadStopAndJoin() {
    if (s_audio_thread) {
        LightEvent_Signal(&s_audio_event);
        threadJoin(s_audio_thread, U64_MAX);
        threadFree(s_audio_thread);
        s_audio_thread = NULL;
    }
}
