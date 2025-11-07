#pragma once

#include "sample.h"
#include <3ds.h>

#define CLEANUP_QUEUE_SIZE 128

typedef struct {
    Sample* samples[CLEANUP_QUEUE_SIZE];
    u32 read_ptr;
    u32 write_ptr;
    LightLock lock;
} CleanupQueue;

void cleanupQueueInit(CleanupQueue* q);
bool cleanupQueuePush(CleanupQueue* q, Sample* s);
Sample* cleanupQueuePop(CleanupQueue* q);
