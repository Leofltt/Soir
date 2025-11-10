#include "sample_bank.h"
#include "audio_utils.h"
#include <string.h>

const char *DEFAULT_SAMPLE_PATHS[DEFAULT_SAMPLE_PATHS_COUNT] = {
    "romfs:/samples/bibop.opus", "romfs:/samples/hatClose.opus", "romfs:/samples/hatOpen.opus",
    "romfs:/samples/kick909.opus", "romfs:/samples/clap808.opus"
};

const char *SAMPLES_FOLDER_PATH = "sdmc:/samples/";

void SampleBankInit(SampleBank *bank) {
    LightLock_Init(&bank->lock);
    for (int i = 0; i < MAX_SAMPLES; i++) {
        bank->samples[i] = NULL;
    }

    for (int i = 0; i < 5; i++) {
        SampleBankLoadSample(bank, i, DEFAULT_SAMPLE_PATHS[i]);
    }
}

void SampleBankDeinit(SampleBank *bank) {
    for (int i = 0; i < MAX_SAMPLES; i++) {
        if (bank->samples[i] != NULL) {
            sample_dec_ref_main_thread(bank->samples[i]);
            bank->samples[i] = NULL;
        }
    }
}

Sample *SampleBankGetSample(SampleBank *bank, int index) {
    if (index < 0 || index >= MAX_SAMPLES) {
        return NULL;
    }

    LightLock_Lock(&bank->lock);
    Sample *sample = bank->samples[index];
    LightLock_Unlock(&bank->lock);

    return sample;
}

const char *SampleBankGetSampleName(SampleBank *bank, int index) {
    if (index < 0 || index >= MAX_SAMPLES) {
        return "Invalid";
    }

    LightLock_Lock(&bank->lock);
    Sample *sample = bank->samples[index];
    LightLock_Unlock(&bank->lock);

    if (sample == NULL) {
        return "Empty";
    }
    return sample_get_name(sample); // This function is safe
}

void SampleBankLoadSample(SampleBank *bank, int index, const char *path) {
    if (index < 0 || index >= MAX_SAMPLES) {
        return;
    }

    bank->samples[index] = sample_create(path);
}

int SampleBankGetLoadedSampleCount(SampleBank *bank) {
    int count = 0;

    LightLock_Lock(&bank->lock);
    for (int i = 0; i < MAX_SAMPLES; i++) {
        if (bank->samples[i] != NULL) {
            count++;
        }
    }
    LightLock_Unlock(&bank->lock);

    return count;
}
