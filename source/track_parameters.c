#include "track_parameters.h"
#include "audio_utils.h"
#include "synth.h"
#include <string.h>

SubSynthParameters defaultSubSynthParameters() {
    SubSynthParameters params;
    memset(&params, 0, sizeof(SubSynthParameters));
    params.env_atk          = 50;
    params.env_dec          = 300;
    params.env_sus_level    = 0.8f;
    params.env_rel          = 100;
    params.env_dur          = 700;
    params.osc_freq         = 220.0f, // A3
        params.osc_waveform = SQUARE;
    params.pulse_width      = 0.5f;
    return params;
};
OpusSamplerParameters defaultOpusSamplerParameters() {
    OpusSamplerParameters params;
    memset(&params, 0, sizeof(OpusSamplerParameters));
    params.env_atk        = 50;
    params.env_dec        = 300;
    params.env_sus_level  = 0.8f;
    params.env_rel        = 100;
    params.env_dur        = 1000;
    params.playback_mode  = ONE_SHOT;
    params.start_position = 0;
    params.sample_index   = 0;
    return params;
};

FMSynthParameters defaultFMSynthParameters() {
    FMSynthParameters params;
    memset(&params, 0, sizeof(FMSynthParameters));
    params.carrier_env_atk       = 50;
    params.carrier_env_dec       = 300;
    params.carrier_env_sus_level = 0.8f;
    params.carrier_env_rel       = 100;
    params.env_dur               = 600;
    params.mod_env_atk           = 50;
    params.mod_env_dec           = 300;
    params.mod_env_sus_level     = 0.8f;
    params.mod_env_rel           = 100;
    params.mod_index             = 1.0f;
    params.mod_depth             = 44.0f;
    params.carrier_freq          = 220.0f;
    params.mod_freq_ratio        = 1.0f;
    params.mod_env_atk           = params.carrier_env_atk;
    params.mod_env_dec           = params.carrier_env_dec;
    params.mod_env_sus_level     = params.carrier_env_sus_level;
    params.mod_env_rel           = params.carrier_env_rel;
    return params;
}

NoiseSynthParameters defaultNoiseSynthParameters() {
    NoiseSynthParameters params;
    memset(&params, 0, sizeof(NoiseSynthParameters));
    params.env_atk       = 20;
    params.env_dec       = 200;
    params.env_sus_level = 0.6f;
    params.env_rel       = 50;
    params.env_dur       = 300;
    return params;
}

TrackParameters defaultTrackParameters(int track_id, void *instrument_data) {
    TrackParameters params;
    memset(&params, 0, sizeof(TrackParameters));
    params.track_id           = track_id;
    params.volume             = 1.0f;    // 0 to 1
    params.pan                = 0.0f;    // -1 to 1, 0 is center
    params.ndsp_filter_cutoff = 8000.0f; // Default to 8000Hz cutoff
    params.ndsp_filter_type   = NDSP_BIQUAD_NONE;
    params.is_muted           = false;
    params.is_soloed          = false;
    params.instrument_data    = instrument_data;
    return params;
};