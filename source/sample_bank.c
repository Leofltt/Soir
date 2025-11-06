#include "sample_bank.h"
#include "audio_utils.h"
#include <string.h>

const char *DEFAULT_SAMPLE_PATHS[DEFAULT_SAMPLE_PATHS_COUNT] = {
    "romfs:/samples/bibop.opus", "romfs:/samples/hatClose.opus", "romfs:/samples/hatOpen.opus",
    "romfs:/samples/kick909.opus", "romfs:/samples/clap808.opus"
};

const char *SAMPLES_FOLDER_PATH = "sdmc:/samples/";

void SampleBankInit(SampleBank *bank) {
    for (int i = 0; i < MAX_SAMPLES; i++) {
        bank->samples[i] = NULL;
    }

    for (int i = 0; i < 5; i++) {
        bank->samples[i] = sample_create(DEFAULT_SAMPLE_PATHS[i]);
    }
}

void SampleBankDeinit(SampleBank *bank) {
    for (int i = 0; i < MAX_SAMPLES; i++) {
        if (bank->samples[i] != NULL) {
            sample_dec_ref(bank->samples[i]);
            bank->samples[i] = NULL;
        }
    }
}

Sample *SampleBankGetSample(SampleBank *bank, int index) {
    if (index < 0 || index >= MAX_SAMPLES) {
        return NULL;
    }
    return bank->samples[index];
}

const char *SampleBankGetSampleName(SampleBank *bank, int index) {
    if (index < 0 || index >= MAX_SAMPLES) {
        return "Invalid";
    }
    if (bank->samples[index] == NULL) {
        return "Empty";
    }
    return sample_get_name(bank->samples[index]);
}

void SampleBankLoadSample(SampleBank *bank, int index, const char *path) {
    if (index < 0 || index >= MAX_SAMPLES) {
        return;
    }

    if (bank->samples[index] != NULL) {
        sample_dec_ref(bank->samples[index]);
    }

    bank->samples[index] = sample_create(path);
}

int SampleBankGetLoadedSampleCount(SampleBank *bank) {
    int count = 0;
    for (int i = 0; i < MAX_SAMPLES; i++) {
        if (bank->samples[i] != NULL) {
            count++;
        }
    }
    return count;
}
