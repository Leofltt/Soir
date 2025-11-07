#ifndef EVENT_H
#define EVENT_H

#include "track_parameters.h"
#include "synth.h"       // For SubSynthParameters
#include "samplers.h"    // For OpusSamplerParameters
#include "noise_synth.h" // For NoiseSynthParameters
#include "track.h"       // For InstrumentType
#include <stdbool.h>

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
    SET_BEATS_PER_BAR

} EventType;

typedef struct {
    EventType type;
    int       track_id; // Used by most events

    union {
        // For TRIGGER_STEP, UPDATE_STEP
        struct {
            TrackParameters base_params;
            InstrumentType  instrument_type;
            union {
                SubSynthParameters    subsynth_params;
                OpusSamplerParameters sampler_params;
                FMSynthParameters     fm_synth_params;
                NoiseSynthParameters  noise_synth_params;
            } instrument_specific_params;
        } step_data;

        // For CLOCK_TICK
        struct {
            int ticks_to_process;
        } clock_data;

        // For TOGGLE_STEP
        struct {
            int step_id;
        } toggle_step_data;

        // For SET_MUTE
        struct {
            bool muted;
        } mute_data;

        // For SET_BPM
        struct {
            float bpm;
        } bpm_data;

        // For SET_BEATS_PER_BAR
        struct {
            int beats;
        } beats_data;

    } data;
} Event;

#endif // EVENT_H