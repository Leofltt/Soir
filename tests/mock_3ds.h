#ifndef MOCK_3DS_H
#define MOCK_3DS_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

// Mock 3DS types
typedef int8_t   s8;
typedef uint8_t  u8;
typedef int16_t  s16;
typedef uint16_t u16;
typedef int32_t  s32;
typedef uint32_t u32;
typedef int64_t  s64;
typedef uint64_t u64;
typedef float    f32;
typedef double   f64;
typedef s32     Result;  // Added Result type

// Mock memory allocation functions
#ifndef linearAlloc
#define linearAlloc malloc
#define linearFree free
#endif

// Mock system constants
#define SCREEN_WIDTH 400
#define SCREEN_HEIGHT 240
#define TOP_SCREEN_WIDTH 400
#define BOTTOM_SCREEN_WIDTH 320

// Mock NDSP types and constants
typedef enum {
    NDSP_FORMAT_STEREO_PCM16 = 0,
    NDSP_FORMAT_MONO_PCM16,
    NDSP_WBUF_DONE
} NDSPFormats;

typedef enum {
    NDSP_INTERP_NONE = 0,
    NDSP_INTERP_LINEAR
} NDSPInterpType;

typedef struct {
    u16* data_pcm16;
    u32 nsamples;
    NDSPFormats format;
    NDSPInterpType interp;
    u32 status;
} ndspWaveBuf;

// Function declarations only
void DSP_FlushDataCache(void* addr, size_t size);
Result ndspChnWaveBufAdd(int channel, ndspWaveBuf* waveBuf);

// Memory allocation macros
#ifndef linearAlloc
#define linearAlloc malloc
#define linearFree free
#endif

// Mock Opus types
typedef struct OggOpusFile OggOpusFile;

// Constants
#define DSP_CHANNELS 24
#define op_read_stereo(file, pcm, samples) 0
#define op_pcm_seek(file, pos) 0
#define op_raw_seek(file, pos) 0

#endif // MOCK_3DS_H