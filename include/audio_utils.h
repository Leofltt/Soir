#ifndef AUDIO_UTILS_H
#define AUDIO_UTILS_H 

#include <math.h>
#include <3ds/types.h>

#ifndef M_PI 
#define M_PI 3.14159265358979323846
#endif

#ifndef M_TWOPI
#define M_TWOPI (2.0 * M_PI)
#endif

#define MAX_AMP_PCM16 0x7FFF
#define BYTESPERSAMPLE 4

#define SAMPLERATE 48000
#define SAMPLESPERBUF (SAMPLERATE * 120 / 1000)
#define NCHANNELS 2

#define K_INT16_MAX (float)MAX_AMP_PCM16
#define K_INT16_MAX_PLUS1 (K_INT16_MAX + 1.)
#define K_FACTOR (1. / K_INT16_MAX_PLUS1)
#define K_INT16_MIN ((-1.) * K_INT16_MAX_PLUS1)

extern float clamp(float d, float min, float max);

extern s16 float_to_int16(float x);

extern float int16_to_float(s16 x);

extern void fillBufferWithZeros(void* audioBuffer, size_t size);

#endif // AUDIO_UTILS_H 