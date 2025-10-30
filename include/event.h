#ifndef EVENT_H
#define EVENT_H

#include "track_parameters.h"
#include "synth.h" // For SubSynthParameters
#include "samplers.h" // For OpusSamplerParameters
#include "track.h" // For InstrumentType

typedef enum {
    TRIGGER_STEP
} EventType;

typedef struct {
    EventType type;
    int track_id;
    TrackParameters base_params; // Base parameters (volume, pan, filter, etc.)
    InstrumentType instrument_type; // To know which union member to use
    union {
        SubSynthParameters subsynth_params;
        OpusSamplerParameters sampler_params;
        // Add other instrument types here as needed
    } instrument_specific_params;
} Event;

#endif // EVENT_H
