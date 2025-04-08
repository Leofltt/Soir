#include "audio_utils.h"

#include <3ds/services/dsp.h>

float clamp(float d, float min, float max) {
    const float t = d < min ? min : d;
    return t > max ? max : t;
};

s16 floatToInt16(float x) {
    s16 converted = (s16) clamp(x * K_INT16_MAX_PLUS1, K_INT16_MIN, K_INT16_MAX);
    return converted;
};

float int16ToFloat(s16 x) {
    float converted = ((float) x) * K_FACTOR;
    return converted;
};

void fillBufferWithZeros(void *audioBuffer, size_t size) {
    u32 *dest = (u32 *) audioBuffer;

    for (int i = 0; i < size; i++) {
        s16 sample = 0;
        dest[i]    = (sample << 16) | (sample & 0xffff);
    }

    DSP_FlushDataCache(audioBuffer, size);
};