
#include "sample_browser.h"
#include "sample_bank.h"
#include <stdio.h>
#include <string.h>
#include <sys/dirent.h>
#include <3ds.h>

static void add_sample(SampleBrowser *browser, const char *path, const char *prefix) {
    if (browser->count >= MAX_BROWSER_SAMPLES) {
        return;
    }

    const char *filename = strrchr(path, '/');
    if (filename) {
        filename++;
    } else {
        filename = path;
    }

    char name[MAX_SAMPLE_NAME_LENGTH];
    snprintf(name, MAX_SAMPLE_NAME_LENGTH, "%s %s", prefix, filename);

    strncpy(browser->names[browser->count], name, MAX_SAMPLE_NAME_LENGTH - 1);
    browser->names[browser->count][MAX_SAMPLE_NAME_LENGTH - 1] = '\0';

    strncpy(browser->paths[browser->count], path, MAX_SAMPLE_PATH_LENGTH - 1);
    browser->paths[browser->count][MAX_SAMPLE_PATH_LENGTH - 1] = '\0';

    browser->count++;
}

void SampleBrowserInit(SampleBrowser *browser) {
    browser->count = 0;

    for (int i = 0; i < 5; i++) {
        add_sample(browser, DEFAULT_SAMPLE_PATHS[i], "I");
    }

    DIR *dir = opendir(SAMPLES_FOLDER_PATH);
    if (dir) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            size_t len = strlen(entry->d_name);
            if (len > 5 && strcmp(entry->d_name + len - 5, ".opus") == 0) {
                char full_path[256];
                snprintf(full_path, sizeof(full_path), "%s%s", SAMPLES_FOLDER_PATH, entry->d_name);
                add_sample(browser, full_path, "SD");
            }
        }
        closedir(dir);
    }
}

void SampleBrowserDeinit(SampleBrowser *browser) {
    // Nothing to do
}

const char *SampleBrowserGetSampleName(SampleBrowser *browser, int index) {
    if (index < 0 || index >= browser->count) {
        return NULL;
    }
    return browser->names[index];
}

const char *SampleBrowserGetSamplePath(SampleBrowser *browser, int index) {
    if (index < 0 || index >= browser->count) {
        return NULL;
    }
    return browser->paths[index];
}

int SampleBrowserGetSampleCount(SampleBrowser *browser) {
    return browser->count;
}
