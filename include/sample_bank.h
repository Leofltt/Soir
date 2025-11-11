#ifndef SAMPLE_BANK_H
#define SAMPLE_BANK_H

#include "sample.h"
#include <3ds.h>

#define MAX_SAMPLES 12
#define DEFAULT_SAMPLE_PATHS_COUNT 5

extern const char *DEFAULT_SAMPLE_PATHS[DEFAULT_SAMPLE_PATHS_COUNT];
extern const char *SAMPLES_FOLDER_PATH;

typedef struct {
    Sample   *samples[MAX_SAMPLES];
    LightLock lock;
} SampleBank;

void    SampleBankInit(SampleBank *bank);
void    SampleBankDeinit(SampleBank *bank);
Sample *SampleBankGetSample(SampleBank *bank, int index);
void    SampleBankGetSampleName(SampleBank *bank, int index, char *buffer, size_t buffer_size);
void    SampleBankLoadSample(SampleBank *bank, int index, const char *path);
int     SampleBankGetLoadedSampleCount(SampleBank *bank);

#endif // SAMPLE_BANK_H
