#ifdef TESTING
#include "../tests/mock_3ds.h"
#else
#include <3ds/ndsp/ndsp.h>
#include <3ds/services/dsp.h>
#include <3ds/types.h>
#endif

#include "audio_utils.h"
#include "synth.h"

void fillSubSynthAudiobuffer(ndspWaveBuf *waveBuf, size_t size, SubSynth *subsynth,
                             float synthvol) {
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
    // REMOVED: ndspChnWaveBufAdd(chan_id, waveBuf);
};