#include "cleanup_queue.h"
#include <string.h>

void cleanupQueueInit(CleanupQueue* q) {
    memset(q->samples, 0, sizeof(q->samples));
    q->read_ptr = 0;
    q->write_ptr = 0;
    LightLock_Init(&q->lock);
}

bool cleanupQueuePush(CleanupQueue* q, Sample* s) {
    LightLock_Lock(&q->lock);
    u32 next_write_ptr = (q->write_ptr + 1) % CLEANUP_QUEUE_SIZE;
    if (next_write_ptr == q->read_ptr) {
        // Queue is full
        LightLock_Unlock(&q->lock);
        return false;
    }
    q->samples[q->write_ptr] = s;
    q->write_ptr = next_write_ptr;
    LightLock_Unlock(&q->lock);
    return true;
}

Sample* cleanupQueuePop(CleanupQueue* q) {
    LightLock_Lock(&q->lock);
    if (q->read_ptr == q->write_ptr) {
        // Queue is empty
        LightLock_Unlock(&q->lock);
        return NULL;
    }
    Sample* s = q->samples[q->read_ptr];
    q->read_ptr = (q->read_ptr + 1) % CLEANUP_QUEUE_SIZE;
    LightLock_Unlock(&q->lock);
    return s;
}
