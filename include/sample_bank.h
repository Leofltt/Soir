#ifndef SAMPLE_BANK_H
#define SAMPLE_BANK_H

#include "sample.h"

#define MAX_SAMPLES 12
#define DEFAULT_SAMPLE_PATHS_COUNT 5

extern const char *DEFAULT_SAMPLE_PATHS[DEFAULT_SAMPLE_PATHS_COUNT];
extern const char *SAMPLES_FOLDER_PATH;

typedef struct {
    Sample *samples[MAX_SAMPLES];
} SampleBank;

void        SampleBankInit(SampleBank *bank);
void        SampleBankDeinit(SampleBank *bank);
Sample     *SampleBankGetSample(SampleBank *bank, int index);
const char *SampleBankGetSampleName(SampleBank *bank, int index);
void        SampleBankLoadSample(SampleBank *bank, int index, const char *path);
int         SampleBankGetLoadedSampleCount(SampleBank *bank);

#endif // SAMPLE_BANK_H
