
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

void        SampleBrowserInit(SampleBrowser *browser);
void        SampleBrowserDeinit(SampleBrowser *browser);
const char *SampleBrowserGetSampleName(const SampleBrowser *browser, int index);
const char *SampleBrowserGetSamplePath(const SampleBrowser *browser, int index);
int         SampleBrowserGetSampleCount(const SampleBrowser *browser);
