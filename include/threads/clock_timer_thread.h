#ifndef CLOCK_TIMER_THREAD_H
#define CLOCK_TIMER_THREAD_H

#include <3ds.h>
#include "clock.h"
#include "track.h"
#include "event_queue.h"

/**
 * @brief Initializes the clock timer thread.
 *
 * This function sets up the necessary shared data pointers for the clock timer thread.
 * It does not start the thread.
 *
 * @param clock_ptr Pointer to the shared Clock object. The thread will read and modify this object.
 *                  Must be protected by clock_lock_ptr. The caller retains ownership.
 * @param clock_lock_ptr Pointer to the lock protecting the shared Clock object.
 * @param tracks_lock_ptr Pointer to the lock protecting the shared Track objects.
 * @param event_queue_ptr Pointer to the event queue for posting sequencer events. The caller
 * retains ownership.
 * @param tracks_ptr Pointer to the array of Track objects. The thread will read this data.
 *                   Must be protected by tracks_lock_ptr. The caller retains ownership.
 * @param should_exit_ptr Pointer to a volatile boolean flag. When set to true, the thread will
 * clean up and exit.
 * @param main_thread_prio The priority of the main application thread, used to calculate the
 * priority of this thread.
 * @return 0 on success, or a libctru error code on failure.
 */
s32 clockTimerThreadsInit(Clock *clock_ptr, LightLock *clock_lock_ptr, EventQueue *event_queue_ptr,
                          volatile bool *should_exit_ptr, s32 main_thread_prio);

/**
 * @brief Starts the clock timer thread.
 *
 * clockTimerThreadsInit() must be called successfully before calling this function.
 *
 * @return 0 on success, or a libctru error code on failure (e.g., if thread creation fails).
 */
s32 clockTimerThreadsStart();

/**
 * @brief Stops the clock timer thread and waits for it to exit.
 *
 * This function signals the thread to exit and then joins it, blocking until the thread has
 * terminated.
 *
 * @return 0 on success, or a libctru error code on failure (e.g., if joining fails).
 */
s32 clockTimerThreadsStopAndJoin();

#endif // CLOCK_TIMER_THREAD_H