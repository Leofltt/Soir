#include "sample.h"
#include "sample_bank.h"
#include "cleanup_queue.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <3ds/allocator/linear.h>

static void         _sample_destroy(Sample *sample);
static CleanupQueue g_cleanup_queue;

void sample_cleanup_init(void) {
    cleanupQueueInit(&g_cleanup_queue);
}

void sample_cleanup_process(void) {
    Sample *s = NULL;
    while ((s = cleanupQueuePop(&g_cleanup_queue)) != NULL) {
        _sample_destroy(s);
    }
}

static void _sample_destroy(Sample *sample) {
    if (!sample) {
        return;
    }
    if (sample->pcm_data) {
        free(sample->pcm_data);
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

    // Replace strdup with malloc + strcpy
    sample->path = malloc(strlen(path) + 1);
    if (!sample->path) {
        free(sample);
        return NULL;
    }
    strcpy(sample->path, path);

    int          err      = 0;
    OggOpusFile *opusFile = op_open_file(path, &err);
    if (err != 0) {
        free(sample->path);
        free(sample);
        return NULL;
    }

    sample->pcm_length              = op_pcm_total(opusFile, -1);
    sample->pcm_data_size_in_frames = sample->pcm_length;
    sample->pcm_data = (int16_t *) malloc(sample->pcm_data_size_in_frames * 2 * sizeof(int16_t));

    if (!sample->pcm_data) {
        op_free(opusFile);
        free(sample->path);
        free(sample);
        return NULL;
    }

    int total_samples_read = 0;
    while (total_samples_read < sample->pcm_data_size_in_frames) {
        int samples_read = op_read_stereo(opusFile, sample->pcm_data + total_samples_read * 2,
                                          sample->pcm_data_size_in_frames - total_samples_read);
        if (samples_read <= 0) {
            break;
        }
        total_samples_read += samples_read;
    }

    op_free(opusFile);

    sample->ref_count = 1;
    LightLock_Init(&sample->lock);

    return sample;
}

void sample_inc_ref(Sample *sample) {
    if (!sample)
        return;
    LightLock_Lock(&sample->lock);
    if (sample->ref_count == INT_MAX) {
        LightLock_Unlock(&sample->lock);
        return; // or abort/log error
    }
    sample->ref_count++;
    LightLock_Unlock(&sample->lock);
}

// This function is called by the audio thread.
// It must NOT call linearFree. It must queue the free.
void sample_dec_ref_audio_thread(Sample *sample) {
    if (!sample)
        return;

    LightLock_Lock(&sample->lock);

    if (sample->ref_count <= 0) {
        LightLock_Unlock(&sample->lock);
        return;
    }

    sample->ref_count--;
    if (sample->ref_count == 0) {
        cleanupQueuePush(&g_cleanup_queue, sample);
    }

    LightLock_Unlock(&sample->lock);
}

// This function is called by the main thread.
// It can (and should) free the memory immediately.
void sample_dec_ref_main_thread(Sample *sample) {
    if (!sample)
        return;

    LightLock_Lock(&sample->lock);

    if (sample->ref_count <= 0) {
        LightLock_Unlock(&sample->lock);
        return;
    }

    sample->ref_count--;
    if (sample->ref_count == 0) {
        cleanupQueuePush(&g_cleanup_queue, sample);
    }

    LightLock_Unlock(&sample->lock);
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