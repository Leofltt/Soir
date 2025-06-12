#ifndef AUDIO_UTILS_H
#define AUDIO_UTILS_H

#ifdef TESTING
#include "../tests/mock_3ds.h"
#else
#include <3ds/types.h>
#endif
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_TWOPI
#define M_TWOPI (2.0 * M_PI)
#endif

#define MAX_AMP_PCM16 0x7FFF
#define BYTESPERSAMPLE 4

#define K_INT16_MAX (float) MAX_AMP_PCM16
#define K_INT16_MAX_PLUS1 (K_INT16_MAX + 1.)
#define K_FACTOR (1. / K_INT16_MAX_PLUS1)
#define K_INT16_MIN ((-1.) * K_INT16_MAX_PLUS1)

#define MIDI_A4 69     // MIDI note number for A4
#define FREQ_A4 440.0f // frequency of A4 in Hz

extern float clamp(float d, float min, float max);

extern s16 floatToInt16(float x);

extern float int16ToFloat(s16 x);

extern void fillBufferWithZeros(void *audioBuffer, size_t size);

extern float midiToHertz(int midiNote);

#endif // AUDIO_UTILS_H