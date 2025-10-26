#include "event_queue.h"

void event_queue_init(EventQueue* q) {
    atomic_init(&q->head, 0);
    atomic_init(&q->tail, 0);
}

bool event_queue_push(EventQueue* q, Event e) {
    // SPSC only: must be called by single producer thread
    int head = atomic_load_explicit(&q->head, memory_order_relaxed);
    int next_head = (head + 1) % EVENT_QUEUE_SIZE;
    if (next_head == atomic_load_explicit(&q->tail, memory_order_acquire)) {
        // Queue is full
        return false;
    }
    q->events[head] = e;
    atomic_store_explicit(&q->head, next_head, memory_order_release);
    return true;
}

bool event_queue_pop(EventQueue* q, Event* e) {
    // SPSC only: must be called by single consumer thread
    int tail = atomic_load_explicit(&q->tail, memory_order_relaxed);
    if (atomic_load_explicit(&q->head, memory_order_acquire) == tail) {
        // Queue is empty
        return false;
    }
    *e = q->events[tail];
    atomic_store_explicit(&q->tail, (tail + 1) % EVENT_QUEUE_SIZE, memory_order_release);
    return true;
}
