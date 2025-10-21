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