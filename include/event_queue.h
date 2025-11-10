#ifndef EVENT_QUEUE_H
#define EVENT_QUEUE_H

#include <stdbool.h>
#include <stdatomic.h>
#include "event.h"
#include <3ds/synchronization.h> // <-- ADD THIS

#define EVENT_QUEUE_SIZE 16

/**
 * @brief A thread-safe, multi-producer, single-consumer (MPSC) circular event queue.
 * The queue has a capacity of EVENT_QUEUE_SIZE - 1.
 */
typedef struct {
    Event      events[EVENT_QUEUE_SIZE];
    atomic_int head;
    atomic_int tail;
    LightLock  lock; 
} EventQueue;

/**
 * @brief Initializes an event queue. Must be called before first use.
 * This function can be called on an existing queue to safely reset it.
 * @param q The event queue to initialize.
 */
void eventQueueInit(EventQueue *q);

/**
 * @brief Pushes an event to the queue.
 * @param q The event queue.
 * @param e The event to push.
 * @return true if the event was pushed successfully, false if the queue was full.
 */
bool eventQueuePush(EventQueue *q, Event e);

/**
 * @brief Pops an event from the queue.
 * @param q The event queue.
 * @param e A pointer to where the event will be stored.
 * @return true if an event was popped successfully, false if the queue was empty.
 *         If the queue is empty, *e is not modified.
 */
bool eventQueuePop(EventQueue *q, Event *e);

#endif // EVENT_QUEUE_H
