#include "track.h"
#include "audio_utils.h"
#include "engine_constants.h"
#include "synth.h"
#include "track_parameters.h"
#include <stdlib.h>
#ifndef TESTING
#include <3ds/ndsp/ndsp.h>
#include <3ds/types.h>
#endif
#include <math.h>
#include <string.h>

static void updateSubSynthFromSequence(SubSynth *synth, SubSynthParameters *params) {
    if (!synth || !params)
        return;

    setWaveform(synth->osc, params->osc_waveform);
    setOscFrequency(synth->osc, params->osc_freq);
    updateEnvelope(synth->env, params->env_atk, params->env_dec, params->env_sus_level,
                   params->env_rel, params->env_dur);
    triggerEnvelope(synth->env);
}

static void updateSamplerFromSequence(Sampler *sampler, OpusSamplerParameters *params) {
    if (!sampler || !params)
        return;

    sampler->start_position = params->start_position;
    sampler->playback_mode  = params->playback_mode;
    sampler->seek_requested = true;
    sampler->finished       = false;
    updateEnvelope(sampler->env, params->env_atk, params->env_dec, params->env_sus_level,
                   params->env_rel, params->env_dur);
    triggerEnvelope(sampler->env);
}

void initializeTrack(Track *track, int chan_id, InstrumentType instrument_type, float rate,
                     u32 num_samples, u32 *audio_buffer) {
    track->chan_id         = chan_id;
    track->instrument_type = instrument_type;
    track->audioBuffer     = audio_buffer;
    track->is_muted        = false;
    track->is_soloed       = false;
    track->fillBlock       = false;
    track->sequencer       = NULL;
    track->instrument_data = NULL;
    track->volume          = 1.0f; // Initialize volume
    track->pan             = 0.0f; // Initialize pan

    // Initialize default parameters
    track->default_parameters = malloc(sizeof(TrackParameters));
    if (track->default_parameters) {
        void *instrument_data = NULL;
        if (instrument_type == SUB_SYNTH) {
            instrument_data = malloc(sizeof(SubSynthParameters));
            if (instrument_data) {
                *((SubSynthParameters *) instrument_data) = defaultSubSynthParameters();
            }
        } else if (instrument_type == OPUS_SAMPLER) {
            instrument_data = malloc(sizeof(OpusSamplerParameters));
            if (instrument_data) {
                *((OpusSamplerParameters *) instrument_data) = defaultOpusSamplerParameters();
            }
        }
        *(track->default_parameters) = defaultTrackParameters(chan_id, instrument_data);
    }

    ndspChnReset(track->chan_id);
    ndspChnSetInterp(track->chan_id, NDSP_INTERP_LINEAR);
    ndspChnSetRate(track->chan_id, rate);
    ndspChnSetFormat(track->chan_id, NDSP_FORMAT_STEREO_PCM16);

    memset(track->mix, 0, sizeof(track->mix));
    track->mix[0] = 1.0;
    track->mix[1] = 1.0;
    ndspChnSetMix(track->chan_id, track->mix);

    track->filter.id            = chan_id;
    track->filter.filter_type   = NDSP_BIQUAD_NONE;
    track->filter.update_params = false;
    track->filter.cutoff_freq   = 1760.f; // some default

    memset(track->waveBuf, 0, sizeof(track->waveBuf));
    track->waveBuf[0].data_vaddr = &track->audioBuffer[0];
    track->waveBuf[0].nsamples   = num_samples;
    track->waveBuf[1].data_vaddr = &track->audioBuffer[num_samples];
    track->waveBuf[1].nsamples   = num_samples;

    fillBufferWithZeros(track->audioBuffer, num_samples * NCHANNELS * 2);

    ndspChnWaveBufAdd(track->chan_id, &track->waveBuf[0]);
    ndspChnWaveBufAdd(track->chan_id, &track->waveBuf[1]);
}

void resetTrack(Track *track) {
    ndspChnReset(track->chan_id);
}

void updateTrackParameters(Track *track, TrackParameters *params) {
    if (!track || !params) {
        return;
    }

    track->is_muted  = params->is_muted;
    track->is_soloed = params->is_soloed;
    track->volume    = params->volume; // Update track volume
    track->pan       = params->pan;    // Update track pan

    // Volume and pan
    // pan is from -1 (L) to 1 (R)
    float pan_rad = (track->pan + 1.0f) * M_PI / 4.0f; // map [-1, 1] to [0, PI/2]
    track->mix[0] = cosf(pan_rad) * track->volume;
    track->mix[1] = sinf(pan_rad) * track->volume;
    ndspChnSetMix(track->chan_id, track->mix);

    // Filter
    if (track->filter.filter_type != params->ndsp_filter_type ||
        track->filter.cutoff_freq != params->ndsp_filter_cutoff) {
        track->filter.filter_type   = params->ndsp_filter_type;
        track->filter.cutoff_freq   = params->ndsp_filter_cutoff;
        track->filter.update_params = true;
    }
}

void updateTrack(Track *track, Clock *clock) {
    if (!track || !track->sequencer || !clock || track->sequencer->steps_per_beat == 0) {
        return;
    }

    int clock_steps_per_seq_step = STEPS_PER_BEAT / track->sequencer->steps_per_beat;
    if (clock_steps_per_seq_step == 0)
        return;

    if ((clock->barBeats->steps - 1) % clock_steps_per_seq_step == 0) {
        SeqStep step = updateSequencer(track->sequencer);
        if (step.active && step.data) {
            updateTrackParameters(track, step.data);
            if (track->instrument_type == SUB_SYNTH) {
                SubSynthParameters *subsynthParams =
                    (SubSynthParameters *) step.data->instrument_data;
                SubSynth *ss = (SubSynth *) track->instrument_data;
                if (subsynthParams && ss) {
                    updateSubSynthFromSequence(ss, subsynthParams);
                }
            } else if (track->instrument_type == OPUS_SAMPLER) {
                OpusSamplerParameters *opusSamplerParams =
                    (OpusSamplerParameters *) step.data->instrument_data;
                Sampler *s = (Sampler *) track->instrument_data;
                if (opusSamplerParams && s) {
                    updateSamplerFromSequence(s, opusSamplerParams);
                }
            }
        }
    }
}
