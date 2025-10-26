#include "track_parameters.h"

SubSynthParameters defaultSubSynthParameters() {
    SubSynthParameters params = { .env_atk       = 50,
                                .env_dec       = 200,
                                .env_sus_level = 0.6,
                                .env_rel       = 300,
                                .env_dur       = 50 };
    return params;
}

OpusSamplerParameters defaultOpusSamplerParameters(const char *path) {
    OpusSamplerParameters params = { .path           = path,
                                     .env_atk        = 50,
                                     .env_dec        = 200,
                                     .env_sus_level  = 0.6,
                                     .env_rel        = 300,
                                     .env_dur        = 50,
                                     .start_position = 0,
                                     .playback_mode  = ONE_SHOT };
    return params;
}

TrackParameters defaultTrackParameters(int track_id, void *instrument_data) {
    TrackParameters params = { .track_id           = track_id,
                               .is_muted           = false,
                               .is_soloed          = false,
                               .volume             = 1.0,
                               .pan                = 0.0,
                               .ndsp_filter_type   = 0,
                               .ndsp_filter_cutoff = 1760.f,
                               .instrument_data   = instrument_data };
    return params;
}