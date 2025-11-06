#ifndef CLOCK_TIMER_THREAD_H
#define CLOCK_TIMER_THREAD_H

#include <3ds.h>
#include "clock.h"
#include "track.h"
#include "event_queue.h"

void clockTimerThreadsInit(Clock *clock_ptr, LightLock *clock_lock_ptr, LightLock *tracks_lock_ptr,
                           EventQueue *event_queue_ptr, Track *tracks_ptr,
                           volatile bool *should_exit_ptr, s32 main_thread_prio);
void clockTimerThreadsStart();
void clockTimerThreadsStopAndJoin();

#endif // CLOCK_TIMER_THREAD_H