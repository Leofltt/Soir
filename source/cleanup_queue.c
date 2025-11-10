#include "cleanup_queue.h"
#include <string.h>
#include <3ds/synchronization.h>

void cleanupQueueInit(CleanupQueue *q) {
    memset(q->samples, 0, sizeof(q->samples));
    atomic_init(&q->read_ptr, 0);
    atomic_init(&q->write_ptr, 0);
    LightLock_Init(&q->lock);
}

// Producer side (Audio Thread) - SPSC
bool cleanupQueuePush(CleanupQueue *q, Sample *s) {
    int write_ptr      = atomic_load_explicit(&q->write_ptr, memory_order_relaxed);
    int next_write_ptr = (write_ptr + 1) % CLEANUP_QUEUE_SIZE;

    if (next_write_ptr == atomic_load_explicit(&q->read_ptr, memory_order_acquire)) {
        // Queue is full
        return false;
    }
    q->samples[write_ptr] = s;
    atomic_store_explicit(&q->write_ptr, next_write_ptr, memory_order_release);

    return true;
}

// Consumer side (Main Thread)
Sample *cleanupQueuePop(CleanupQueue *q) {
    int read_ptr = atomic_load_explicit(&q->read_ptr, memory_order_relaxed);

    if (atomic_load_explicit(&q->write_ptr, memory_order_acquire) == read_ptr) {
        // Queue is empty
        return NULL;
    }

    Sample *s = q->samples[read_ptr];
    atomic_store_explicit(&q->read_ptr, (read_ptr + 1) % CLEANUP_QUEUE_SIZE, memory_order_release);

    return s;
}
