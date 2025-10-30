#ifndef EVENT_QUEUE_H
#define EVENT_QUEUE_H

#include <stdbool.h>
#include <stdatomic.h>
#include "track_parameters.h"

#define EVENT_QUEUE_SIZE 16

typedef enum { NOTE_ON, NOTE_OFF } EventType;

typedef struct {
    EventType type;
    // The event queue does not own this pointer. The caller is responsible for
    // ensuring the lifetime of the pointed-to data exceeds the time the event
    // is in the queue. Can be NULL.
    TrackParameters params;
} Event;

/**
 * @brief A thread-safe, single-producer, single-consumer (SPSC) circular event queue.
 * The queue has a capacity of EVENT_QUEUE_SIZE - 1.
 */
typedef struct {
    Event      events[EVENT_QUEUE_SIZE];
    atomic_int head;
    atomic_int tail;
} EventQueue;

/**
 * @brief Initializes an event queue. Must be called before first use.
 * This function can be called on an existing queue to safely reset it.
 * @param q The event queue to initialize.
 */
void event_queue_init(EventQueue *q);

/**
 * @brief Pushes an event to the queue.
 * @param q The event queue.
 * @param e The event to push.
 * @return true if the event was pushed successfully, false if the queue was full.
 */
bool event_queue_push(EventQueue *q, Event e);

/**
 * @brief Pops an event from the queue.
 * @param q The event queue.
 * @param e A pointer to where the event will be stored.
 * @return true if an event was popped successfully, false if the queue was empty.
 *         If the queue is empty, *e is not modified.
 */
bool event_queue_pop(EventQueue *q, Event *e);

#endif // EVENT_QUEUE_H
