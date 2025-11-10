#ifndef SAMPLE_H
#define SAMPLE_H

#ifdef TESTING
#include "mock_3ds.h"
#else
#include <opusfile.h>
#include <3ds/synchronization.h>
#endif

typedef struct {
    char      *path;
    int16_t   *pcm_data;
    size_t     pcm_data_size_in_frames;
    opus_int64 pcm_length;
    int        ref_count;
    LightLock  lock;
} Sample;

Sample *sample_create(const char *path);
void    sample_inc_ref(Sample *sample);
void    sample_dec_ref_audio_thread(Sample *sample);
void    sample_dec_ref_main_thread(Sample *sample);

void        sample_cleanup_init(void);
void        sample_cleanup_process(void);
const char *sample_get_name(const Sample *sample);

#endif // SAMPLE_H