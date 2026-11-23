#ifndef PROJECT_H
#define PROJECT_H

#include "track.h"
#include "clock.h"
#include "sample_bank.h"

#define MAX_PROJECTS 128
#define MAX_PROJECT_NAME_LEN 64

bool saveTrackToFile(const char *name, int track_idx, Track *tracks, bool create_project,
                     Clock *clock, SampleBank *sample_bank);
bool loadTrackFromFile(const char *name, Track *track);
bool saveSequenceToFile(const char *name, Track *tracks, Clock *clock);
bool loadSequenceFromFile(const char *name, Track *tracks, Clock *clock);
bool saveProject(const char *name, Track *tracks, Clock *clock, SampleBank *sample_bank);
bool loadProject(const char *name, Track *tracks, Clock *clock, SampleBank *sample_bank);

int  getProjectList(const char *dirpath, const char *ext,
                    char list[MAX_PROJECTS][MAX_PROJECT_NAME_LEN]);
void createProjectDirs(void);

#endif // PROJECT_H
