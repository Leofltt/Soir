#ifndef SAMPLE_BANK_H
#define SAMPLE_BANK_H

#include "sample.h"

#define MAX_SAMPLES 12

typedef struct {
    Sample *samples[MAX_SAMPLES];
} SampleBank;

void        SampleBank_init(SampleBank *bank);
void        SampleBank_deinit(SampleBank *bank);
Sample     *SampleBank_get_sample(SampleBank *bank, int index);
const char *SampleBank_get_sample_name(SampleBank *bank, int index);
void        SampleBank_load_sample(SampleBank *bank, int index, const char *path);

#endif // SAMPLE_BANK_H
