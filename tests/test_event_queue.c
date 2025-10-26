#include "event_queue.h"
#include "unity.h"
#include <stdint.h>

void test_event_queue_init_should_set_head_and_tail_to_zero(void) {
    EventQueue q;
    event_queue_init(&q);
    TEST_ASSERT_EQUAL(0, q.head);
    TEST_ASSERT_EQUAL(0, q.tail);
}

void test_event_queue_push_and_pop_should_work_correctly(void) {
    EventQueue q;
    event_queue_init(&q);
    TrackParameters tp = { .track_id = 1 };
    Event e_push = { .type = NOTE_ON, .params = tp };
    
    bool pushed = event_queue_push(&q, e_push);
    TEST_ASSERT_TRUE(pushed);

    Event e_pop;
    bool popped = event_queue_pop(&q, &e_pop);
    TEST_ASSERT_TRUE(popped);
    TEST_ASSERT_EQUAL(e_push.type, e_pop.type);
    TEST_ASSERT_EQUAL(e_push.params.track_id, e_pop.params.track_id);
}

void test_event_queue_should_not_push_when_full(void) {
    EventQueue q;
    event_queue_init(&q);

    for (int i = 0; i < EVENT_QUEUE_SIZE - 1; i++) {
        TrackParameters tp = { .track_id = i };
        Event e = { .type = NOTE_ON, .params = tp };
        bool pushed = event_queue_push(&q, e);
        TEST_ASSERT_TRUE(pushed);
    }

    TrackParameters tp_full = { .track_id = 99 };
    Event e_full = { .type = NOTE_ON, .params = tp_full };
    bool pushed = event_queue_push(&q, e_full);
    TEST_ASSERT_FALSE(pushed);
}

void test_event_queue_should_not_pop_when_empty(void) {
    EventQueue q;
    event_queue_init(&q);
    Event e;
    bool popped = event_queue_pop(&q, &e);
    TEST_ASSERT_FALSE(popped);
}

void test_event_queue_wraparound_should_work_correctly(void) {
    EventQueue q;
    event_queue_init(&q);

    // Fill the queue
    for (int i = 0; i < EVENT_QUEUE_SIZE - 1; i++) {
        TrackParameters tp = { .track_id = i };
        Event e = { .type = NOTE_ON, .params = tp };
        bool pushed = event_queue_push(&q, e);
        TEST_ASSERT_TRUE(pushed);
    }

    // Pop some events to make space at the beginning
    for (int i = 0; i < 5; i++) {
        Event e;
        bool popped = event_queue_pop(&q, &e);
        TEST_ASSERT_TRUE(popped);
    }

    // Push more events to force wraparound
    for (int i = 0; i < 5; i++) {
        TrackParameters tp = { .track_id = i + 20 };
        Event e = { .type = NOTE_OFF, .params = tp };
        bool pushed = event_queue_push(&q, e);
        TEST_ASSERT_TRUE(pushed);
    }

    // Pop remaining events and check their values
    for (int i = 5; i < EVENT_QUEUE_SIZE - 1; i++) {
        Event e;
        bool popped = event_queue_pop(&q, &e);
        TEST_ASSERT_TRUE(popped);
        TEST_ASSERT_EQUAL(NOTE_ON, e.type);
        TEST_ASSERT_EQUAL(i, e.params.track_id);
    }

    for (int i = 0; i < 5; i++) {
        Event e;
        bool popped = event_queue_pop(&q, &e);
        TEST_ASSERT_TRUE(popped);
        TEST_ASSERT_EQUAL(NOTE_OFF, e.type);
        TEST_ASSERT_EQUAL(i + 20, e.params.track_id);
    }

    // Queue should be empty now
    Event e;
    bool popped = event_queue_pop(&q, &e);
    TEST_ASSERT_FALSE(popped);
}
