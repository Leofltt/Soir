#ifndef AUDIO_THREAD_H
#define AUDIO_THREAD_H

#include <3ds.h>
#include "track.h"
#include "event_queue.h"
#include "sample_bank.h"
#include "engine_constants.h"

// Define STACK_SIZE if it's not already defined globally
#ifndef STACK_SIZE
#define STACK_SIZE (N_TRACKS * 32 * 1024) // Assuming N_TRACKS is defined in engine_constants.h
#endif

/**
 * @brief Initializes the audio thread.
 *
 * This function sets up the necessary shared data pointers for the audio thread.
 * It does not start the thread.
 *
 * @param tracks_ptr Pointer to the array of Track objects. The thread will read and modify this
 * data. Must be protected by tracks_lock_ptr. The caller retains ownership.
 * @param tracks_lock_ptr Pointer to the lock protecting the shared Track objects.
 * @param event_queue_ptr Pointer to the event queue for receiving sequencer events. The caller
 * retains ownership.
 * @param sample_bank_ptr Pointer to the sample bank. The caller retains ownership.
 * @param should_exit_ptr Pointer to a volatile boolean flag. When set to true, the thread will
 * clean up and exit.
 * @param main_thread_prio The priority of the main application thread, used to calculate the
 * priority of this thread.
 * @return 0 on success, or a libctru error code on failure.
 */
s32 audioThreadInit(Track *tracks_ptr, EventQueue *event_queue_ptr, SampleBank *sample_bank_ptr,
                    Clock *clock_ptr, LightLock *clock_lock_ptr, volatile bool *should_exit_ptr,
                    s32 main_thread_prio);

/**
 * @brief Starts the audio thread.
 *
 * audioThreadInit() must be called successfully before calling this function.
 *
 * @return 0 on success, or a libctru error code on failure (e.g., if thread creation fails).
 */
s32 audioThreadStart();

/**
 * @brief Signals the audio thread to wake up from its event wait.
 *
 * This is used to ensure the thread is not sleeping when we need it to process
 * the should_exit flag.
 */
void audioThreadSignal();

/**
 * @brief Stops the audio thread and waits for it to exit.
 *
 * This function signals the thread to exit and then joins it, blocking until the thread has
 * terminated.
 *
 * @return 0 on success, or a libctru error code on failure (e.g., if joining fails).
 */
s32 audioThreadJoin();

#endif // AUDIO_THREAD_H
