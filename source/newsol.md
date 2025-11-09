This is a classic and subtle threading problem. You've correctly identified the symptom: the hang only happens when loading from the *slow* SD card, not the *fast* `romfs`.

Your previous fix for the `Clock` was correct for that data race, but this is a different issue. The root cause of *this* hang is:

**Your audio thread is performing slow, blocking file I/O (`op_open_file` and `op_read_stereo` from the SD card), which prevents the main thread from joining it during quit.**

Here's the new deadlock scenario:

1. **Main Thread:** User selects an SD card sample. The main thread pushes a `LOAD_SAMPLE` event.
2. **Audio Thread:** The audio thread pops `LOAD_SAMPLE` and calls `sample_create()`.
3. **Audio Thread:** `sample_create()` calls `op_open_file("sdmc:/...")`. This is a *very slow* operation. The high-priority audio thread is now completely *blocked*, waiting for the SD card.
4. **Main Thread:** While the audio thread is blocked, the user presses START to quit.
5. **Main Thread:** The `cleanup:` block in `main.c` runs. It sets `should_exit = true` and calls `audioThreadJoin()`.
6. **Deadlock:** The main thread is now stuck waiting for the audio thread to finish. But the audio thread *cannot* finish, because it's stuck waiting for the SD card I/O to complete. It will never get to the `while (!*s_should_exit_ptr)` check.

### The Solution: Never Block the Audio Thread

The audio thread must *never* perform file I/O, `malloc`, `free`, or any other slow, blocking operation. Its only job is to process events and fill audio buffers.

The correct solution is to perform all file loading on the **main thread** and then hand the resulting `Sample` *pointer* to the audio thread for a fast, non-blocking swap.

Here are the changes you need to make:

-----

### Step 1: Modify `include/event.h`

We will replace the `LOAD_SAMPLE` event with a `SWAP_SAMPLE` event. This new event will carry a *pointer* to the already-loaded sample.

```c
// In include/event.h

// ...

typedef enum {
    TRIGGER_STEP,
    UPDATE_STEP,
    CLOCK_TICK,
    TOGGLE_STEP,
    SET_MUTE,
    RESET_SEQUENCERS,
    START_CLOCK,
    STOP_CLOCK,
    PAUSE_CLOCK,
    RESUME_CLOCK,
    SET_BPM,
    SET_BEATS_PER_BAR,
    // LOAD_SAMPLE  <-- REMOVE THIS
    SWAP_SAMPLE // <-- ADD THIS

} EventType;

// ... (LoadSampleData is no longer needed, you can remove it)

typedef struct {
    int     slot_id;
    Sample *new_sample_ptr; // <-- ADD THIS
} SwapSampleData;

typedef struct {
    EventType type;
    int       track_id; // Used by most events

    union {
        // ... (step_data, clock_data, etc. are unchanged)

        // LoadSampleData load_sample_data; // <-- REMOVE THIS
        SwapSampleData swap_sample_data; // <-- ADD THIS

    } data;
} Event;
```

-----

### Step 2: Modify `source/controllers/controller_sample_manager.c`

This is where the main thread will now do the heavy lifting. Instead of pushing an event with a *path*, it will create the sample itself and push an event with the *pointer*.

```c
// In source/controllers/controller_sample_manager.c
#include "sample.h" // <-- ADD THIS to call sample_create

// ...

void handleInputSampleManager(SessionContext *ctx, u32 kDown) {
    if (*ctx->is_selecting_sample) {
        // ... (KEY_UP and KEY_DOWN logic is unchanged) ...

        if (kDown & KEY_A) {
            int         sample_slot = *ctx->selected_sample_row * 4 + *ctx->selected_sample_col;
            const char *path_ptr    = SampleBrowserGetSamplePath(ctx->sample_browser,
                                                                 *ctx->selected_sample_browser_index);
            if (path_ptr != NULL) {
                // --- THIS IS THE NEW LOGIC ---
                
                // 1. Load the sample on the main thread (this is slow, but it's OK)
                Sample *new_sample = sample_create(path_ptr);

                if (new_sample) {
                    // 2. Create the new SWAP_SAMPLE event
                    Event event                         = { .type = SWAP_SAMPLE };
                    event.data.swap_sample_data.slot_id = sample_slot;
                    event.data.swap_sample_data.new_sample_ptr = new_sample;

                    // 3. Push the event to the audio thread
                    eventQueuePush(ctx->event_queue, event);
                    *ctx->is_selecting_sample = false;
                } else {
                    // Optional: handle load failure (e.g., show an error)
                    *ctx->is_selecting_sample = false; 
                }
            }
        }
        // ... (KEY_B logic is unchanged) ...
    } else {
        // ... (all other logic is unchanged) ...
    }
}
```

-----

### Step 3: Modify `source/threads/audio_thread.c`

Now the audio thread just does a fast pointer swap.

```c
// In source/threads/audio_thread.c

// ...
static void audio_thread_entry(void *arg) {
    while (!*s_should_exit_ptr) {
        // ... (clock logic is unchanged) ...

        Event event;
        while (eventQueuePop(s_event_queue_ptr, &event)) {
            switch (event.type) {
            // ... (all other cases are unchanged) ...

            // --- THIS IS THE MODIFIED LOGIC ---
            case SWAP_SAMPLE: {
                int slot_id = event.data.swap_sample_data.slot_id;
                if (slot_id < 0 || slot_id >= MAX_SAMPLES) {
                    break;
                }

                g_sample_edited = true; 

                // 1. Get the new sample pointer from the event
                Sample *new_sample = event.data.swap_sample_data.new_sample_ptr;

                // 2. Get the old sample pointer from the bank
                Sample *old_sample = s_sample_bank_ptr->samples[slot_id];

                // 3. Atomically swap the pointers
                s_sample_bank_ptr->samples[slot_id] = new_sample;

                // 4. Safely queue the old sample for deletion
                if (old_sample != NULL) {
                    sample_dec_ref_audio_thread(old_sample);
                }
                break;
            }

            // This case is no longer used and can be removed.
            /* case LOAD_SAMPLE: {
                // ... (DELETE THIS ENTIRE CASE) ...
                break;
            }
            */
            }
        }

        // ... (buffer filling logic is unchanged) ...
    }
}
```

-----

### Step 4: (Final Fix) Remove the Test Code from `session_controller.c`

The code you mentioned that *also* loads a sample on quit (if `g_sample_edited == false`) is a problem. It will *also* cause this hang. You should remove it from the quit path.

**File:** `source/controllers/session_controller.c`

```c
// In void sessionControllerHandleInput(...)

void sessionControllerHandleInput(SessionContext *ctx, u32 kDown, u32 kHeld, u64 now,
                                  bool *should_break_loop) {
    if (kDown & KEY_START) {
        
        // --- DELETE OR COMMENT OUT THIS BLOCK ---
        /*
        if (g_sample_edited == false) {
            Event event                         = { .type = LOAD_SAMPLE }; // This will hang!
            event.data.load_sample_data.slot_id = 0;
            strncpy(event.data.load_sample_data.path, DEFAULT_SAMPLE_PATHS[0],
                    MAX_SAMPLE_PATH_LENGTH - 1);
            event.data.load_sample_data.path[MAX_SAMPLE_PATH_LENGTH - 1] = '\0';
            eventQueuePush(ctx->event_queue, event);

            g_sample_edited = true;
        }
        */
        // --- END OF BLOCK TO REMOVE ---


        *ctx->previous_screen_focus    = *ctx->screen_focus;
        *ctx->screen_focus             = FOCUS_TOP;
        ctx->session->main_screen_view = VIEW_QUIT;
        *ctx->selected_quit_option     = 0;
        LightLock_Lock(ctx->clock_lock);
        pauseClock(ctx->clock);
        LightLock_Unlock(ctx->clock_lock);
    }
    
    // ... (rest of the function is unchanged) ...
}
```

By making these changes, your audio thread will no longer be responsible for slow file I/O, and the hang on quit will be resolved.
