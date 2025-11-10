#include "threads/audio_thread.h"
#include "audio_utils.h"
#include "synth.h"
#include "samplers.h"
#include "noise_synth.h"
#include "envelope.h"
#include "engine_constants.h"
#include <3ds/ndsp/ndsp.h> // Added for ndspChnWaveBufGet
#include <stdio.h>
#include <string.h>
#include <opusfile.h>

extern bool g_sample_edited;

// Static global variables for this module
static Thread     s_audio_thread;
static LightEvent s_audio_event;

// Pointers to shared state from main
static Track         *s_tracks_ptr       = NULL;
static EventQueue    *s_event_queue_ptr  = NULL;
static SampleBank    *s_sample_bank_ptr  = NULL;
static Clock         *s_clock_ptr        = NULL;
static LightLock     *s_clock_lock_ptr   = NULL;
static volatile bool *s_should_exit_ptr  = NULL;
static s32            s_main_thread_prio = 0;

static void audio_callback(void *data) {
    LightEvent_Signal(&s_audio_event);
}

// Helper function to process events on the audio thread
static void processSequencerTick() {
    s_clock_ptr->barBeats->steps++;
    int totBeats                     = (s_clock_ptr->barBeats->steps - 1) / STEPS_PER_BEAT;
    s_clock_ptr->barBeats->bar       = totBeats / s_clock_ptr->barBeats->beats_per_bar;
    s_clock_ptr->barBeats->beat      = (totBeats % s_clock_ptr->barBeats->beats_per_bar);
    s_clock_ptr->barBeats->deltaStep = (s_clock_ptr->barBeats->steps - 1) % STEPS_PER_BEAT;

    for (int track_idx = 0; track_idx < N_TRACKS; track_idx++) {
        Track *track = &s_tracks_ptr[track_idx];
        if (!track || !track->sequencer || track->sequencer->steps_per_beat == 0) {
            continue;
        }

        int clock_steps_per_seq_step = STEPS_PER_BEAT / track->sequencer->steps_per_beat;
        if (clock_steps_per_seq_step == 0)
            continue;

        if ((s_clock_ptr->barBeats->steps - 1) % clock_steps_per_seq_step == 0) {
            SeqStep step = updateSequencer(track->sequencer);
            if (step.active && !track->is_muted && step.data) {
                Event event                      = { .type = TRIGGER_STEP, .track_id = track_idx };
                event.data.step_data.base_params = *step.data;
                event.data.step_data.instrument_type = track->instrument_type;

                if (track->instrument_type == SUB_SYNTH) {
                    memcpy(&event.data.step_data.instrument_specific_params.subsynth_params,
                           step.data->instrument_data, sizeof(SubSynthParameters));
                } else if (track->instrument_type == OPUS_SAMPLER) {
                    memcpy(&event.data.step_data.instrument_specific_params.sampler_params,
                           step.data->instrument_data, sizeof(OpusSamplerParameters));
                } else if (track->instrument_type == FM_SYNTH) {
                    memcpy(&event.data.step_data.instrument_specific_params.fm_synth_params,
                           step.data->instrument_data, sizeof(FMSynthParameters));
                } else if (track->instrument_type == NOISE_SYNTH) {
                    memcpy(&event.data.step_data.instrument_specific_params.noise_synth_params,
                           step.data->instrument_data, sizeof(NoiseSynthParameters));
                }
                eventQueuePush(s_event_queue_ptr, event);
            }
        }
    }
}

static void audio_thread_entry(void *arg) {
    while (!*s_should_exit_ptr) {
        LightLock_Lock(s_clock_lock_ptr);
        int ticks_to_process = updateClock(s_clock_ptr);

        if (ticks_to_process > 0) {
            // Clock ticked. We call processSequencerTick directly.
            // It will push TRIGGER_STEP events onto our *own* queue (MPSC push is safe).
            for (int i = 0; i < ticks_to_process; i++) {
                processSequencerTick();
            }
        }
        LightLock_Unlock(s_clock_lock_ptr);

        Event event;
        while (eventQueuePop(s_event_queue_ptr, &event)) {
            switch (event.type) {
            case CLOCK_TICK: {
                break;
            }
            case TRIGGER_STEP:
            case UPDATE_STEP: {
                Track *track = &s_tracks_ptr[event.track_id];
                if (!track)
                    break;
                updateTrackParameters(track, &event.data.step_data.base_params);

                if (event.data.step_data.instrument_type == SUB_SYNTH) {
                    SubSynthParameters *subsynthParams =
                        &event.data.step_data.instrument_specific_params.subsynth_params;
                    SubSynth *ss = (SubSynth *) track->instrument_data;
                    if (subsynthParams && ss) {
                        setWaveform(ss->osc, subsynthParams->osc_waveform);
                        setPulseWidth(ss->osc, subsynthParams->pulse_width);
                        setOscFrequency(ss->osc, subsynthParams->osc_freq);
                        updateEnvelope(ss->env, subsynthParams->env_atk, subsynthParams->env_dec,
                                       subsynthParams->env_sus_level, subsynthParams->env_rel,
                                       subsynthParams->env_dur);
                        if (event.type == TRIGGER_STEP) {
                            triggerEnvelope(ss->env);
                        }
                    }
                } else if (event.data.step_data.instrument_type == OPUS_SAMPLER) {
                    OpusSamplerParameters *opusSamplerParams =
                        &event.data.step_data.instrument_specific_params.sampler_params;
                    Sampler *s = (Sampler *) track->instrument_data;
                    if (opusSamplerParams && s) {
                        Sample *new_sample =
                            SampleBankGetSample(s_sample_bank_ptr, opusSamplerParams->sample_index);
                        if (new_sample != s->sample) {
                            sample_inc_ref(new_sample);
                            sample_dec_ref_audio_thread(s->sample);
                            s->sample = new_sample;
                        }
                        s->start_position = opusSamplerParams->start_position;
                        s->playback_mode  = opusSamplerParams->playback_mode;
                        s->current_frame  = s->start_position / NCHANNELS;
                        s->finished       = false;
                        updateEnvelope(s->env, opusSamplerParams->env_atk,
                                       opusSamplerParams->env_dec, opusSamplerParams->env_sus_level,
                                       opusSamplerParams->env_rel, opusSamplerParams->env_dur);
                        if (event.type == TRIGGER_STEP) {
                            triggerEnvelope(s->env);
                        }
                    }
                } else if (event.data.step_data.instrument_type == FM_SYNTH) {
                    FMSynthParameters *fmSynthParams =
                        &event.data.step_data.instrument_specific_params.fm_synth_params;
                    FMSynth *fs = (FMSynth *) track->instrument_data;
                    if (fmSynthParams && fs) {
                        FMOpSetCarrierFrequency(fs->fm_op, fmSynthParams->carrier_freq);
                        FMOpSetModRatio(fs->fm_op, fmSynthParams->mod_freq_ratio);
                        FMOpSetModIndex(fs->fm_op, fmSynthParams->mod_index);
                        FMOpSetModDepth(fs->fm_op, fmSynthParams->mod_depth);
                        updateEnvelope(fs->carrierEnv, fmSynthParams->carrier_env_atk,
                                       fmSynthParams->carrier_env_dec,
                                       fmSynthParams->carrier_env_sus_level,
                                       fmSynthParams->carrier_env_rel, fmSynthParams->env_dur);
                        updateEnvelope(fs->fm_op->mod_envelope, fmSynthParams->mod_env_atk,
                                       fmSynthParams->mod_env_dec, fmSynthParams->mod_env_sus_level,
                                       fmSynthParams->mod_env_rel, fmSynthParams->env_dur);
                        if (event.type == TRIGGER_STEP) {
                            triggerEnvelope(fs->carrierEnv);
                            triggerEnvelope(fs->fm_op->mod_envelope);
                        }
                    }
                } else if (event.data.step_data.instrument_type == NOISE_SYNTH) {
                    NoiseSynthParameters *noiseSynthParams =
                        &event.data.step_data.instrument_specific_params.noise_synth_params;
                    NoiseSynth *ns = (NoiseSynth *) track->instrument_data;
                    if (noiseSynthParams && ns) {
                        updateEnvelope(ns->env, noiseSynthParams->env_atk,
                                       noiseSynthParams->env_dec, noiseSynthParams->env_sus_level,
                                       noiseSynthParams->env_rel, noiseSynthParams->env_dur);
                        if (event.type == TRIGGER_STEP) {
                            triggerEnvelope(ns->env);
                        }
                    }
                }
                break;
            }
            case TOGGLE_STEP: {
                Track *track = &s_tracks_ptr[event.track_id];
                if (track && track->sequencer && event.data.toggle_step_data.step_id < 16) {
                    track->sequencer->steps[event.data.toggle_step_data.step_id].active =
                        !track->sequencer->steps[event.data.toggle_step_data.step_id].active;
                }
                break;
            }
            case SET_MUTE: {
                Track *track = &s_tracks_ptr[event.track_id];
                if (track) {
                    track->is_muted = event.data.mute_data.muted;
                }
                break;
            }
            case RESET_SEQUENCERS: {
                LightLock_Lock(s_clock_lock_ptr);
                for (int i = 0; i < N_TRACKS; i++) {
                    if (s_tracks_ptr[i].sequencer) {
                        s_tracks_ptr[i].sequencer->cur_step = 0;
                    }
                }
                LightLock_Unlock(s_clock_lock_ptr);
                break;
            }

            case START_CLOCK:
                LightLock_Lock(s_clock_lock_ptr);
                startClock(s_clock_ptr);
                LightLock_Unlock(s_clock_lock_ptr);
                break;
            case STOP_CLOCK:
                LightLock_Lock(s_clock_lock_ptr);
                stopClock(s_clock_ptr);
                LightLock_Unlock(s_clock_lock_ptr);
                break;
            case PAUSE_CLOCK:
                LightLock_Lock(s_clock_lock_ptr);
                pauseClock(s_clock_ptr);
                LightLock_Unlock(s_clock_lock_ptr);
                break;
            case RESUME_CLOCK:
                LightLock_Lock(s_clock_lock_ptr);
                resumeClock(s_clock_ptr);
                LightLock_Unlock(s_clock_lock_ptr);
                break;
            case SET_BPM:
                LightLock_Lock(s_clock_lock_ptr);
                setBpm(s_clock_ptr, event.data.bpm_data.bpm);
                LightLock_Unlock(s_clock_lock_ptr);
                break;
            case SET_BEATS_PER_BAR:
                LightLock_Lock(s_clock_lock_ptr);
                setBeatsPerBar(s_clock_ptr, event.data.beats_data.beats);
                LightLock_Unlock(s_clock_lock_ptr);
                break;
            case SWAP_SAMPLE: {
                int slot_id = event.data.swap_sample_data.slot_id;
                if (slot_id < 0 || slot_id >= MAX_SAMPLES) {
                    // new_sample_ptr was malloc'd, we must queue it for freeing
                    sample_dec_ref_audio_thread(event.data.swap_sample_data.new_sample_ptr);
                    break;
                }

                g_sample_edited    = true;
                Sample *new_sample = event.data.swap_sample_data.new_sample_ptr;

                // --- ADD LOCKS ---
                LightLock_Lock(&s_sample_bank_ptr->lock);
                Sample *old_sample                  = s_sample_bank_ptr->samples[slot_id];
                s_sample_bank_ptr->samples[slot_id] = new_sample;
                LightLock_Unlock(&s_sample_bank_ptr->lock);
                // --- END LOCKS ---

                if (old_sample != NULL) {
                    // This is now safe, it just queues the old sample
                    sample_dec_ref_audio_thread(old_sample);
                }
                break;
            }
            }
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
                    fillSamplerAudioBuffer(waveBuf, waveBuf->nsamples, sampler);
                } else if (s_tracks_ptr[i].instrument_type == FM_SYNTH) {
                    FMSynth *fm_synth = (FMSynth *) s_tracks_ptr[i].instrument_data;
                    fillFMSynthAudiobuffer(waveBuf, waveBuf->nsamples, fm_synth);
                } else if (s_tracks_ptr[i].instrument_type == NOISE_SYNTH) {
                    NoiseSynth *noise_synth = (NoiseSynth *) s_tracks_ptr[i].instrument_data;
                    fillNoiseSynthAudiobuffer(waveBuf, waveBuf->nsamples, noise_synth);
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

s32 audioThreadInit(Track *tracks_ptr, EventQueue *event_queue_ptr, SampleBank *sample_bank_ptr,
                    Clock *clock_ptr, LightLock *clock_lock_ptr, volatile bool *should_exit_ptr,
                    s32 main_thread_prio) {
    s_tracks_ptr       = tracks_ptr;
    s_event_queue_ptr  = event_queue_ptr;
    s_sample_bank_ptr  = sample_bank_ptr;
    s_clock_ptr        = clock_ptr;
    s_clock_lock_ptr   = clock_lock_ptr;
    s_should_exit_ptr  = should_exit_ptr;
    s_main_thread_prio = main_thread_prio;
    LightEvent_Init(&s_audio_event, RESET_ONESHOT);
    ndspSetCallback(audio_callback, NULL);
    return 0;
}

s32 audioThreadStart() {
    s_audio_thread =
        threadCreate(audio_thread_entry, NULL, STACK_SIZE, s_main_thread_prio - 2, -2, true);
    if (s_audio_thread == NULL) {
        *s_should_exit_ptr = true;
        return -1;
    }
    return 0;
}

void audioThreadSignal() {
    if (s_audio_thread) {
        LightEvent_Signal(&s_audio_event);
    }
}

s32 audioThreadJoin() {
    Result res = 0;
    if (s_audio_thread) {
        res = threadJoin(s_audio_thread, U64_MAX);
        threadFree(s_audio_thread);
        s_audio_thread = NULL;
    }
    return res;
}
