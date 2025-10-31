#include "track_parameters.h"
#include "synth.h"

SubSynthParameters defaultSubSynthParameters() {
    SubSynthParameters params = { .env_atk       = 50,
                                  .env_dec       = 300,
                                  .env_sus_time  = 550,
                                  .env_sus_level = 0.8f,
                                  .env_rel       = 100,
                                  .env_dur       = 1000,
                                  .osc_freq      = 440.0f, // A4
                                  .osc_waveform  = SINE };
    return params;
};
OpusSamplerParameters defaultOpusSamplerParameters() {
    OpusSamplerParameters params = { .env_atk        = 50,
                                     .env_dec        = 300,
                                     .env_sus_time   = 550,
                                     .env_sus_level  = 0.8f,
                                     .env_rel        = 100,
                                     .env_dur        = 1000,
                                     .playback_mode  = ONE_SHOT,
                                     .start_position = 0,
                                     .sample_index   = 0 };
    return params;
};
TrackParameters defaultTrackParameters(int track_id, void *instrument_data) {
    TrackParameters params = { .track_id           = track_id,
                               .volume             = 1.0f,     // 0 to 1
                               .pan                = 0.0f,     // -1 to 1, 0 is center
                               .ndsp_filter_cutoff = 20000.0f, // Default to max cutoff
                               .ndsp_filter_type   = NDSP_BIQUAD_NONE,
                               // .reverb_level        = 0.0f,
                               // .delay_level         = 0.0f,
                               .is_muted        = false,
                               .is_soloed       = false,
                               .instrument_data = instrument_data };
    return params;
};