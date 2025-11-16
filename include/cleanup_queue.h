#pragma once

#include "sample.h"
#include <3ds.h>
#include <stdatomic.h>

#define CLEANUP_QUEUE_SIZE 512

typedef struct {
    Sample    *samples[CLEANUP_QUEUE_SIZE];
    atomic_int write_ptr;
    atomic_int read_ptr;
} SampleCleanupQueue;

void    sampleCleanupQueueInit(SampleCleanupQueue *q);
bool    sampleCleanupQueuePush(SampleCleanupQueue *q, Sample *s);
Sample *sampleCleanupQueuePop(SampleCleanupQueue *q);
