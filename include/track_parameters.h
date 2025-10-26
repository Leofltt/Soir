#ifndef TRACK_PARAMETERS_H
#define TRACK_PARAMETERS_H

#ifdef TESTING
#include "../tests/mock_3ds.h"
#else
#include <3ds/types.h>
#include <opusfile.h>
#endif

// Forward-declare the instrument-specific parameter structs
struct SubSynthParameters;
struct OpusSamplerParameters;

typedef struct TrackParameters {
    int   track_id;
    bool  is_muted;
    bool  is_soloed;
    float volume;
    float pan;
    int   ndsp_filter_type;
    float ndsp_filter_cutoff;
    // This pointer is not owned by the struct. The caller is responsible for
    // managing the memory.
    void *instrument_data; // Pointer to one of the specific param structs below
} TrackParameters;

typedef struct SubSynthParameters {
    int   osc_waveform;
    float osc_freq;
    float env_atk;
    float env_dec;
    float env_sus_level;
    float env_sus_time;
    float env_rel;
    float env_dur;
} SubSynthParameters;

typedef enum {
    ONE_SHOT,
    LOOP,
} PlaybackMode;

typedef struct OpusSamplerParameters {
    const char *path;
    PlaybackMode playback_mode;
    int64_t      start_position;
    float env_atk;
    float env_dec;
    float env_sus_level;
    float env_sus_time;
    float env_rel;
    float env_dur;
} OpusSamplerParameters;


extern TrackParameters defaultTrackParameters(int track_id, void *instrument_data);
extern SubSynthParameters defaultSubSynthParameters();
extern OpusSamplerParameters defaultOpusSamplerParameters(const char *path);

#endif // TRACK_PARAMETERS_H