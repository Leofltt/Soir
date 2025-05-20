#include "track.h"
#include "audio_utils.h"
#include "engine_constants.h"

#include <3ds.h>

void initializeTrack(Track *track) {
    float rate      = track->instrument_type == SYNTHESISER ? SAMPLERATE : OPUSSAMPLESPERFBUF;
    u32   n_samples = track->instrument_type == SYNTHESISER ? SAMPLESPERBUF : OPUSSAMPLESPERFBUF;
    ndspChnReset(track->chan_id);
    ndspChnSetInterp(track->chan_id, NDSP_INTERP_LINEAR);
    ndspChnSetRate(track->chan_id, rate);
    ndspChnSetFormat(track->chan_id, NDSP_FORMAT_STEREO_PCM16);
    ndspChnSetMix(track->chan_id, track->mix);

    track->waveBuf[0].data_vaddr = &track->audioBuffer[0];
    track->waveBuf[0].nsamples   = n_samples;
    track->waveBuf[1].data_vaddr = &track->audioBuffer[n_samples];
    track->waveBuf[1].nsamples   = n_samples;

    fillBufferWithZeros(track->audioBuffer, n_samples * NCHANNELS);

    ndspChnWaveBufAdd(track->chan_id, &(track->waveBuf)[0]);
    ndspChnWaveBufAdd(track->chan_id, &(track->waveBuf)[1]);
}

void resetTrack(Track *track) {
    ndspChnReset(track->chan_id);
}