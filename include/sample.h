#ifndef SAMPLE_H
#define SAMPLE_H

#ifndef TESTING
#include <opusfile.h>
#endif

typedef struct {
    char        *path;
    OggOpusFile *opusFile;
    opus_int64   pcm_length;
} Sample;

Sample     *sample_create(const char *path);
void        sample_destroy(Sample *sample);
const char *sample_get_name(const Sample *sample);

#endif // SAMPLE_H