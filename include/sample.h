#ifndef SAMPLE_H
#define SAMPLE_H

#ifndef TESTING
#include <opusfile.h>
#include <3ds/synchronization.h>
#endif

typedef struct {
    char        *path;
    OggOpusFile *opusFile;
    opus_int64   pcm_length;
    int          ref_count;
    LightLock    lock;
} Sample;

Sample     *sample_create(const char *path);
void        sample_inc_ref(Sample *sample);
void        sample_dec_ref(Sample *sample);
const char *sample_get_name(const Sample *sample);

#endif // SAMPLE_H