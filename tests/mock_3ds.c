#include "mock_3ds.h"

u64 mock_system_tick = 0;

void mock_set_system_tick(u64 ticks) {
    mock_system_tick = ticks;
}

void mock_advance_system_tick(u64 ticks) {
    mock_system_tick += ticks;
}

u64 svcGetSystemTick(void) {
    return mock_system_tick;
}

void DSP_FlushDataCache(void *addr, size_t size) {
    // Mock implementation
}

Result ndspChnWaveBufAdd(int channel, ndspWaveBuf *waveBuf) {
    // Mock implementation
    return 0;
}

// Mocks to compile project.c on host tests
#include "track.h"
#include "sample_bank.h"

void updateTrackParameters(Track *track, TrackParameters *params) {
    // Mock implementation
}

Sample *SampleBankGetSample(SampleBank *bank, int index) {
    return NULL;
}

void SampleBankLoadSample(SampleBank *bank, int index, const char *path) {
    // Mock implementation
}

#include "track_parameters.h"

SubSynthParameters defaultSubSynthParameters() {
    SubSynthParameters p = { 0 };
    return p;
}
OpusSamplerParameters defaultOpusSamplerParameters() {
    OpusSamplerParameters p = { 0 };
    return p;
}
FMSynthParameters defaultFMSynthParameters() {
    FMSynthParameters p = { 0 };
    return p;
}
NoiseSynthParameters defaultNoiseSynthParameters() {
    NoiseSynthParameters p = { 0 };
    return p;
}
TrackParameters defaultTrackParameters(int track_id, void *instrument_data) {
    TrackParameters p = { 0 };
    p.track_id        = track_id;
    p.instrument_data = instrument_data;
    return p;
}

void sample_dec_ref_main_thread(Sample *sample) {
    // Mock
}