#ifndef SAMPLE_H
#define SAMPLE_H

#include <opusfile.h>

typedef struct {
    char        *path;
    OggOpusFile *opusFile;
} Sample;

Sample     *sample_create(const char *path);
void        sample_destroy(Sample *sample);
const char *sample_get_name(const Sample *sample);

#endif // SAMPLE_H
