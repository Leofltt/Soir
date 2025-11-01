#include "sample.h"
#include <stdlib.h>
#include <string.h>

Sample *sample_create(const char *path) {
    Sample *sample = (Sample *) malloc(sizeof(Sample));
    if (!sample) {
        return NULL;
    }

    sample->path = strdup(path);
    if (!sample->path) {
        free(sample);
        return NULL;
    }

    int err          = 0;
    sample->opusFile = op_open_file(path, &err);
    if (err != 0) {
        free(sample->path);
        free(sample);
        return NULL;
    }

    sample->pcm_length = op_pcm_total(sample->opusFile, -1);

    return sample;
}

void sample_destroy(Sample *sample) {
    if (!sample) {
        return;
    }
    if (sample->opusFile) {
        op_free(sample->opusFile);
    }
    if (sample->path) {
        free(sample->path);
    }
    free(sample);
}

static char sample_name_buffer[64];

const char *sample_get_name(const Sample *sample) {
    if (!sample || !sample->path) {
        return "---";
    }

    const char *last_slash = strrchr(sample->path, '/');
    const char *name_start = last_slash ? last_slash + 1 : sample->path;

    strncpy(sample_name_buffer, name_start, sizeof(sample_name_buffer) - 1);
    sample_name_buffer[sizeof(sample_name_buffer) - 1] = '\0';

    char *dot = strrchr(sample_name_buffer, '.');
    if (dot) {
        *dot = '\0';
    }

    return sample_name_buffer;
}