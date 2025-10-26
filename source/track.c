#include "track.h"
#include "audio_utils.h"
#include "engine_constants.h"
#include "synth.h"
#include "track_parameters.h"
#ifndef TESTING
#include <3ds/ndsp/ndsp.h>
#include <3ds/types.h>
#endif
#include <math.h>
#include <string.h>



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
    event_queue_init(&track->event_queue);

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

    // Volume and pan
    // pan is from -1 (L) to 1 (R)
    float pan_rad = (params->pan + 1.0f) * M_PI / 4.0f; // map [-1, 1] to [0, PI/2]
    track->mix[0] = cosf(pan_rad) * params->volume;
    track->mix[1] = sinf(pan_rad) * params->volume;
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
    if (clock_steps_per_seq_step == 0) return;

    if ((clock->barBeats->steps - 1) % clock_steps_per_seq_step == 0) {
        SeqStep step = updateSequencer(track->sequencer);
        if (step.active && step.data) {
            updateTrackParameters(track, step.data);
            Event e = { .type = NOTE_ON, .params = *step.data };
            event_queue_push(&track->event_queue, e);
        }
    }
}