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
static LightLock     *s_tracks_lock_ptr  = NULL;
static EventQueue    *s_event_queue_ptr  = NULL;
static Track         *s_tracks_ptr       = NULL;
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
            LightLock_Lock(s_tracks_lock_ptr);
            for (int i = 0; i < ticks_to_process; i++) {
                int totBeats                = s_clock_ptr->barBeats->steps / STEPS_PER_BEAT;
                s_clock_ptr->barBeats->bar  = totBeats / s_clock_ptr->barBeats->beats_per_bar;
                s_clock_ptr->barBeats->beat = (totBeats % s_clock_ptr->barBeats->beats_per_bar);
                s_clock_ptr->barBeats->deltaStep = s_clock_ptr->barBeats->steps % STEPS_PER_BEAT;
                s_clock_ptr->barBeats->steps++;

                for (int track_idx = 0; track_idx < N_TRACKS; track_idx++) {
                    Track *track = &s_tracks_ptr[track_idx];
                    if (!track || !track->sequencer || !s_clock_ptr ||
                        track->sequencer->steps_per_beat == 0) {
                        continue;
                    }

                    int clock_steps_per_seq_step =
                        STEPS_PER_BEAT / track->sequencer->steps_per_beat;
                    if (clock_steps_per_seq_step == 0)
                        continue;

                    if ((s_clock_ptr->barBeats->steps - 1) % clock_steps_per_seq_step == 0) {
                        SeqStep step = updateSequencer(track->sequencer);
                        if (step.active && !track->is_muted && step.data) {
                            Event event = { .type            = TRIGGER_STEP,
                                            .track_id        = track_idx,
                                            .base_params     = *step.data,
                                            .instrument_type = track->instrument_type };

                            if (track->instrument_type == SUB_SYNTH) {
                                memcpy(&event.instrument_specific_params.subsynth_params,
                                       step.data->instrument_data, sizeof(SubSynthParameters));
                            } else if (track->instrument_type == OPUS_SAMPLER) {
                                memcpy(&event.instrument_specific_params.sampler_params,
                                       step.data->instrument_data, sizeof(OpusSamplerParameters));
                            }
                            event_queue_push(s_event_queue_ptr, event);
                        }
                    }
                }
            }
            LightLock_Unlock(s_tracks_lock_ptr);
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

void clock_timer_threads_init(Clock *clock_ptr, LightLock *clock_lock_ptr,
                              LightLock *tracks_lock_ptr, EventQueue *event_queue_ptr,
                              Track *tracks_ptr, volatile bool *should_exit_ptr,
                              s32 main_thread_prio) {
    s_clock_ptr        = clock_ptr;
    s_clock_lock_ptr   = clock_lock_ptr;
    s_tracks_lock_ptr  = tracks_lock_ptr;
    s_event_queue_ptr  = event_queue_ptr;
    s_tracks_ptr       = tracks_ptr;
    s_should_exit_ptr  = should_exit_ptr;
    s_main_thread_prio = main_thread_prio;
    LightEvent_Init(&s_clock_event, RESET_ONESHOT);
}

void clock_timer_threads_start() {
    s_clock_thread =
        threadCreate(clock_thread_entry, NULL, STACK_SIZE, s_main_thread_prio - 1, -2, true);
    if (s_clock_thread == NULL) {
        printf("Failed to create clock thread\n");
        // Handle error appropriately, maybe set a flag or exit
    }

    s_timer_thread =
        threadCreate(timer_thread_entry, NULL, STACK_SIZE, s_main_thread_prio - 1, -2, true);
    if (s_timer_thread == NULL) {
        printf("Failed to create timer thread\n");
        // Handle error appropriately
    }
}

void clock_timer_threads_stop_and_join() {
    if (s_clock_thread) {
        LightEvent_Signal(&s_clock_event);
        threadJoin(s_clock_thread, U64_MAX);
        threadFree(s_clock_thread);
        s_clock_thread = NULL;
    }

    if (s_timer_thread) {
        threadJoin(s_timer_thread, U64_MAX);
        threadFree(s_timer_thread);
        s_timer_thread = NULL;
    }
}