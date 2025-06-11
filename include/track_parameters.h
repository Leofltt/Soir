#ifndef TRACK_PARAMETERS_H
#define TRACK_PARAMETERS_H

#include "filters.h"
#include "polybleposc.h"
#include "samplers.h"

// a poor attempt at serialisable track parameters for each step

typedef struct {
    int            track_id;
    float          volume; // volume and pan together determine mix[0] and mix[1]
    float          pan;
    float          ndsp_filter_cutoff;
    NdspFilterType ndsp_filter_type;
    // float          reverb_level;
    // float          delay_level;
    bool  is_muted;
    bool  is_soloed;
    void *instrument_data;
} TrackParameters;

typedef struct {
    int      env_atk;
    int      env_dec;
    int      env_sus_time;
    float    env_sus_level;
    int      env_rel;
    int      env_dur;
    float    osc_freq;
    Waveform osc_waveform;
} SubSynthParameters;

typedef struct {
    int          env_atk;
    int          env_dec;
    int          env_sus_time;
    float        env_sus_level;
    int          env_rel;
    int          env_dur;
    OggOpusFile *audiofile;
    PlaybackMode playback_mode;
    int64_t      start_position;
} OpusSamplerParameters;

extern SubSynthParameters    defaultSubSynthParameters();
extern OpusSamplerParameters defaultOpusSamplerParameters(OggOpusFile *audiofile);
extern TrackParameters       defaultTrackParameters(int track_id, void *instrument_data);

#endif // TRACK_PARAMETERS_H