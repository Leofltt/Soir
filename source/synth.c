#ifdef TESTING
#include "../tests/mock_3ds.h"
#else
#include <3ds/ndsp/ndsp.h>
#include <3ds/services/dsp.h>
#include <3ds/types.h>
#endif

#include "audio_utils.h"
#include "synth.h"
#include "track.h"

static void updateSubSynthFromSequence(SubSynth *synth, SubSynthParameters *params) {
    if (!synth || !params)
        return;

    setWaveform(synth->osc, params->osc_waveform);
    setOscFrequency(synth->osc, params->osc_freq);
    updateEnvelope(synth->env, params->env_atk, params->env_dec, params->env_sus_level, params->env_rel, params->env_dur);
    triggerEnvelope(synth->env);
}

void fillSubSynthAudiobuffer(struct Track *track, ndspWaveBuf *waveBuf, size_t size, float synthvol) {
    if (!track || !track->instrument_data) {
        return;
    }

    SubSynth *subsynth = (SubSynth *) track->instrument_data;
    Event e;
    while (event_queue_pop(&track->event_queue, &e)) {
        if (e.type == NOTE_ON) {
            if (e.params.instrument_data) {
                updateSubSynthFromSequence(subsynth, (SubSynthParameters *) e.params.instrument_data);
            }
        }
    }

    u32 *dest = (u32 *) waveBuf->data_pcm16;

    for (int i = 0; i < size; i++) {
        float next_sam  = nextOscillatorSample(subsynth->osc);
        next_sam        = fmaxf(-1.0f, fminf(1.0f, next_sam));
        float env_value = nextEnvelopeSample(subsynth->env);
        env_value       = fmaxf(0.0f, fminf(1.0f, env_value));
        next_sam *= env_value;
        next_sam *= synthvol;
        s16 sample = floatToInt16(next_sam);
        dest[i]    = (sample << 16) | (sample & 0xffff);
    }

    DSP_FlushDataCache(waveBuf->data_pcm16, size);
    ndspChnWaveBufAdd(track->chan_id, waveBuf);
};