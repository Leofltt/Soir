#include "sample.h"
#include <stdlib.h>
#include <string.h>

static void _sample_destroy(Sample *sample) {
    if (!sample) {
        return;
    }
    if (sample->opusFile) {
        if (strncmp(sample->path, "sdmc:/", 6) == 0) {
            svcSleepThread(10000000); // 10ms delay to allow pending I/O to complete
        }
        op_free(sample->opusFile);
    }
    if (sample->path) {
        free(sample->path);
    }
    free(sample);
}

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
    sample->ref_count  = 1;
    LightLock_Init(&sample->lock);

    return sample;
}

void sample_inc_ref(Sample *sample) {
    if (!sample)
        return;
    LightLock_Lock(&sample->lock);
    sample->ref_count++;
    LightLock_Unlock(&sample->lock);
}

void sample_dec_ref(Sample *sample) {
    if (!sample)
        return;

    LightLock_Lock(&sample->lock); // Acquire lock first

    // Now, check ref_count safely within the lock
    if (sample->ref_count <= 0) {
        LightLock_Unlock(&sample->lock); // Release lock before returning
        return;                          // Already freed or invalid, do not proceed
    }

    sample->ref_count--;
    if (sample->ref_count == 0) {
        LightLock_Unlock(&sample->lock); // Release lock before destroying
        _sample_destroy(sample);
    } else {
        LightLock_Unlock(&sample->lock);
    }
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