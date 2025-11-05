#ifndef AUDIO_THREAD_H
#define AUDIO_THREAD_H

#include <3ds.h>
#include "track.h"
#include "event_queue.h"
#include "sample_bank.h"

// Define STACK_SIZE if it's not already defined globally
#ifndef STACK_SIZE
#define STACK_SIZE (N_TRACKS * 32 * 1024) // Assuming N_TRACKS is defined in engine_constants.h
#endif

void audio_thread_init(Track *tracks_ptr, LightLock *tracks_lock_ptr, EventQueue *event_queue_ptr,
                       SampleBank *sample_bank_ptr, volatile bool *should_exit_ptr,
                       s32 main_thread_prio);
void audio_thread_start();
void audio_thread_stop_and_join();

#endif // AUDIO_THREAD_H
