#include "clock_timer_thread.h"
#include "clock.h"
#include "event_queue.h"
#include "track.h"
#include "sequencer.h"
#include "engine_constants.h"

#include <3ds.h>
#include <stdio.h>
#include <string.h>

#define STACK_SIZE (N_TRACKS * 32 * 1024)

static Thread     s_clock_thread;
static Thread     s_timer_thread;
static LightEvent s_clock_event;

static Clock         *s_clock_ptr        = NULL;
static LightLock     *s_clock_lock_ptr   = NULL;
static EventQueue    *s_event_queue_ptr  = NULL;
static volatile bool *s_should_exit_ptr  = NULL;
static s32            s_main_thread_prio = 0;

static void clock_thread_entry(void *arg) {
    while (!*s_should_exit_ptr) {
        LightEvent_Wait(&s_clock_event);
        if (*s_should_exit_ptr)
            break;

        LightLock_Lock(s_clock_lock_ptr);
        int ticks_to_process = updateClock(s_clock_ptr);

        if (ticks_to_process > 0) {
            Event event                            = { .type = CLOCK_TICK };
            event.data.clock_data.ticks_to_process = ticks_to_process;
            eventQueuePush(s_event_queue_ptr, event);
        }
        LightLock_Unlock(s_clock_lock_ptr);
    }
}

static void timer_thread_entry(void *arg) {
    while (!*s_should_exit_ptr) {
        svcSleepThread(100000);
        LightEvent_Signal(&s_clock_event);
    }
}

s32 clockTimerThreadsInit(Clock *clock_ptr, LightLock *clock_lock_ptr, EventQueue *event_queue_ptr,
                          volatile bool *should_exit_ptr, s32 main_thread_prio) {
    s_clock_ptr        = clock_ptr;
    s_clock_lock_ptr   = clock_lock_ptr;
    s_event_queue_ptr  = event_queue_ptr;
    s_should_exit_ptr  = should_exit_ptr;
    s_main_thread_prio = main_thread_prio;
    LightEvent_Init(&s_clock_event, RESET_ONESHOT);
    return 0;
}

s32 clockTimerThreadsStart() {
    s_clock_thread =
        threadCreate(clock_thread_entry, NULL, STACK_SIZE, s_main_thread_prio - 1, -2, true);
    if (s_clock_thread == NULL) {
        return -1;
    }

    s_timer_thread =
        threadCreate(timer_thread_entry, NULL, STACK_SIZE, s_main_thread_prio - 1, -2, true);
    if (s_timer_thread == NULL) {
        // The created clock_thread will wait on an event and never exit, causing a hang on join.
        // We must signal it to exit properly.
        *s_should_exit_ptr = true;
        LightEvent_Signal(&s_clock_event);
        threadJoin(s_clock_thread, U64_MAX);
        threadFree(s_clock_thread);
        s_clock_thread = NULL;
        return -1;
    }
    return 0;
}

s32 clockTimerThreadsStopAndJoin() {
    Result res = 0;
    Result join_res;

    if (s_clock_thread) {
        LightEvent_Signal(&s_clock_event);
        join_res = threadJoin(s_clock_thread, U64_MAX);
        if (R_FAILED(join_res)) {
            res = join_res;
        }
        threadFree(s_clock_thread);
        s_clock_thread = NULL;
    }

    if (s_timer_thread) {
        join_res = threadJoin(s_timer_thread, U64_MAX);
        if (R_FAILED(join_res) && R_SUCCEEDED(res)) {
            res = join_res;
        }
        threadFree(s_timer_thread);
        s_timer_thread = NULL;
    }
    return res;
}