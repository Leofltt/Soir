#include "sample_bank.h"
#include <string.h>

static const char *DEFAULT_SAMPLE_PATHS[5] = {
    "romfs:/samples/bibop.opus", "romfs:/samples/hatClose.opus", "romfs:/samples/hatOpen.opus",
    "romfs:/samples/kick909.opus", "romfs:/samples/clap808.opus"
};

void SampleBank_init(SampleBank *bank) {
    for (int i = 0; i < MAX_SAMPLES; i++) {
        bank->samples[i] = NULL;
    }

    for (int i = 0; i < 5; i++) {
        bank->samples[i] = sample_create(DEFAULT_SAMPLE_PATHS[i]);
    }
}

void SampleBank_deinit(SampleBank *bank) {
    for (int i = 0; i < MAX_SAMPLES; i++) {
        if (bank->samples[i] != NULL) {
            sample_destroy(bank->samples[i]);
            bank->samples[i] = NULL;
        }
    }
}

Sample *SampleBank_get_sample(SampleBank *bank, int index) {
    if (index < 0 || index >= MAX_SAMPLES) {
        return NULL;
    }
    return bank->samples[index];
}

const char *SampleBank_get_sample_name(SampleBank *bank, int index) {
    if (index < 0 || index >= MAX_SAMPLES) {
        return "Invalid";
    }
    if (bank->samples[index] == NULL) {
        return "Empty";
    }
    return sample_get_name(bank->samples[index]);
}

void SampleBank_load_sample(SampleBank *bank, int index, const char *path) {
    if (index < 0 || index >= MAX_SAMPLES) {
        return;
    }

    if (bank->samples[index] != NULL) {
        sample_destroy(bank->samples[index]);
    }

    bank->samples[index] = sample_create(path);
}
