
#pragma once

#include "sample.h"

#define MAX_BROWSER_SAMPLES 128
#define MAX_SAMPLE_NAME_LENGTH 64
#define MAX_SAMPLE_PATH_LENGTH 512

typedef struct {
    char names[MAX_BROWSER_SAMPLES][MAX_SAMPLE_NAME_LENGTH];
    char paths[MAX_BROWSER_SAMPLES][MAX_SAMPLE_PATH_LENGTH];
    int  count;
} SampleBrowser;

void        SampleBrowser_init(SampleBrowser *browser);
void        SampleBrowser_deinit(SampleBrowser *browser);
const char *SampleBrowser_get_sample_name(SampleBrowser *browser, int index);
const char *SampleBrowser_get_sample_path(SampleBrowser *browser, int index);
int         SampleBrowser_get_sample_count(SampleBrowser *browser);
