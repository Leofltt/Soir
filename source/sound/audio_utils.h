#ifndef AUDIO_UTILS_H
#define AUDIO_UTILS_H 

#include <math.h>

#ifndef M_PI 
#define M_PI 3.14159265358979323846
#endif

#ifndef M_TWOPI
#define M_TWOPI (2.0 * M_PI)
#endif

#define MAX_AMP_PCM16 0x7FFF
#define BYTESPERSAMPLE 4

static const float SAMPLERATE = 48000.;
static const int SAMPLESPERBUF = SAMPLERATE * 120 / 1000;
static const int NCHANNELS = 2;

static const float K_INT16_MAX = (float) MAX_AMP_PCM16; 
static const float K_INT16_MAX_PLUS1 = K_INT16_MAX + 1.; 
static const float K_FACTOR = 1. / K_INT16_MAX_PLUS1;
static const float K_INT16_MIN = (-1.) * K_INT16_MAX_PLUS1;

float clamp(float d, float min, float max) {
  const float t = d < min ? min : d;
  return t > max ? max : t;
};

s16 float_to_int16(float x) {
    s16 converted = (s16) clamp(x * K_INT16_MAX_PLUS1, K_INT16_MIN, K_INT16_MAX);
    return converted;
};

float int16_to_float(s16 x){    
    float converted = ((float) x) * K_FACTOR;
    return converted;
};

#endif // AUDIO_UTILS_H 