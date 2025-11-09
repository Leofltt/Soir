#pragma once

#include "sample.h"
#include <3ds.h>
#include <stdatomic.h>

#define CLEANUP_QUEUE_SIZE 128

typedef struct {
    Sample    *samples[CLEANUP_QUEUE_SIZE];
    atomic_int write_ptr;
    atomic_int read_ptr;
} CleanupQueue;

void    cleanupQueueInit(CleanupQueue *q);
bool    cleanupQueuePush(CleanupQueue *q, Sample *s);
Sample *cleanupQueuePop(CleanupQueue *q);
