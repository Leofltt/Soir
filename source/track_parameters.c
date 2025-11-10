#include "track_parameters.h"
#include "audio_utils.h"
#include "synth.h"

SubSynthParameters defaultSubSynthParameters() {
    SubSynthParameters params = { .env_atk       = 50,
                                  .env_dec       = 300,
                                  .env_sus_level = 0.8f,
                                  .env_rel       = 100,
                                  .env_dur       = 1000,
                                  .osc_freq      = 220.0f, // A3
                                  .osc_waveform  = SINE,
                                  .pulse_width   = 0.5f };
    return params;
};
OpusSamplerParameters defaultOpusSamplerParameters() {
    OpusSamplerParameters params = { .env_atk        = 50,
                                     .env_dec        = 300,
                                     .env_sus_level  = 0.8f,
                                     .env_rel        = 100,
                                     .env_dur        = 1000,
                                     .playback_mode  = ONE_SHOT,
                                     .start_position = 0,
                                     .sample_index   = 0 };
    return params;
};

FMSynthParameters defaultFMSynthParameters() {
    FMSynthParameters params = { .carrier_env_atk       = 50,
                                 .carrier_env_dec       = 300,
                                 .carrier_env_sus_level = 0.8f,
                                 .carrier_env_rel       = 100,
                                 .env_dur               = 1000,
                                 .mod_env_atk           = 50,
                                 .mod_env_dec           = 300,
                                 .mod_env_sus_level     = 0.8f,
                                 .mod_env_rel           = 100,
                                 .mod_index             = 1.0f,
                                 .mod_depth             = 100.0f,
                                 .carrier_freq          = 220.0f,
                                 .mod_freq_ratio        = 1.0f };
    params.mod_env_atk       = params.carrier_env_atk;
    params.mod_env_dec       = params.carrier_env_dec;
    params.mod_env_sus_level = params.carrier_env_sus_level;
    params.mod_env_rel       = params.carrier_env_rel;
    return params;
}

NoiseSynthParameters defaultNoiseSynthParameters() {
    return (NoiseSynthParameters) {
        .env_atk       = 20,
        .env_dec       = 200,
        .env_sus_level = 0.6f,
        .env_rel       = 50,
        .env_dur       = 300,
    };
}

TrackParameters defaultTrackParameters(int track_id, void *instrument_data) {
    TrackParameters params = { .track_id           = track_id,
                               .volume             = 1.0f,    // 0 to 1
                               .pan                = 0.0f,    // -1 to 1, 0 is center
                               .ndsp_filter_cutoff = 8000.0f, // Default to 8000Hz cutoff
                               .ndsp_filter_type   = NDSP_BIQUAD_NONE,
                               .is_muted           = false,
                               .is_soloed          = false,
                               .instrument_data    = instrument_data };
    return params;
};