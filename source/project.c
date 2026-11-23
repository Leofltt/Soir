#include "project.h"
#include "engine_constants.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#ifdef TESTING
#define PATH_PREFIX "./"
#else
#define PATH_PREFIX "sdmc:/"
#endif

// Helper to write a track to an open file stream
static bool saveTrackToStream(FILE *f, Track *track) {
    if (!f || !track)
        return false;

    // Write track-level settings
    if (fwrite(&track->volume, sizeof(float), 1, f) != 1)
        return false;
    if (fwrite(&track->pan, sizeof(float), 1, f) != 1)
        return false;

    u8 is_muted  = track->is_muted ? 1 : 0;
    u8 is_soloed = track->is_soloed ? 1 : 0;
    if (fwrite(&is_muted, sizeof(u8), 1, f) != 1)
        return false;
    if (fwrite(&is_soloed, sizeof(u8), 1, f) != 1)
        return false;

    int filter_type = (int) track->filter.filter_type;
    if (fwrite(&filter_type, sizeof(int), 1, f) != 1)
        return false;
    if (fwrite(&track->filter.cutoff_freq, sizeof(float), 1, f) != 1)
        return false;

    int inst_type = (int) track->instrument_type;
    if (fwrite(&inst_type, sizeof(int), 1, f) != 1)
        return false;

    // Write sequencer details
    if (!track->sequencer) {
        int zero = 0;
        if (fwrite(&zero, sizeof(int), 1, f) != 1)
            return false; // n_beats
        if (fwrite(&zero, sizeof(int), 1, f) != 1)
            return false; // steps_per_beat
        return true;
    }

    if (fwrite(&track->sequencer->n_beats, sizeof(int), 1, f) != 1)
        return false;
    if (fwrite(&track->sequencer->steps_per_beat, sizeof(int), 1, f) != 1)
        return false;

    int num_steps = track->sequencer->n_beats * track->sequencer->steps_per_beat;
    for (int i = 0; i < num_steps; i++) {
        SeqStep *step   = &track->sequencer->steps[i];
        u8       active = step->active ? 1 : 0;
        if (fwrite(&active, sizeof(u8), 1, f) != 1)
            return false;

        if (step->data) {
            // Write TrackParameters for the step
            if (fwrite(&step->data->volume, sizeof(float), 1, f) != 1)
                return false;
            if (fwrite(&step->data->pan, sizeof(float), 1, f) != 1)
                return false;
            if (fwrite(&step->data->ndsp_filter_cutoff, sizeof(float), 1, f) != 1)
                return false;

            int step_filter_type = (int) step->data->ndsp_filter_type;
            if (fwrite(&step_filter_type, sizeof(int), 1, f) != 1)
                return false;

            u8 step_muted  = step->data->is_muted ? 1 : 0;
            u8 step_soloed = step->data->is_soloed ? 1 : 0;
            if (fwrite(&step_muted, sizeof(u8), 1, f) != 1)
                return false;
            if (fwrite(&step_soloed, sizeof(u8), 1, f) != 1)
                return false;

            // Write instrument parameters based on instrument_type
            if (track->instrument_type == SUB_SYNTH) {
                SubSynthParameters *p = (SubSynthParameters *) step->data->instrument_data;
                if (p) {
                    if (fwrite(p, sizeof(SubSynthParameters), 1, f) != 1)
                        return false;
                } else {
                    SubSynthParameters dp = defaultSubSynthParameters();
                    if (fwrite(&dp, sizeof(SubSynthParameters), 1, f) != 1)
                        return false;
                }
            } else if (track->instrument_type == OPUS_SAMPLER) {
                OpusSamplerParameters *p = (OpusSamplerParameters *) step->data->instrument_data;
                if (p) {
                    if (fwrite(p, sizeof(OpusSamplerParameters), 1, f) != 1)
                        return false;
                } else {
                    OpusSamplerParameters dp = defaultOpusSamplerParameters();
                    if (fwrite(&dp, sizeof(OpusSamplerParameters), 1, f) != 1)
                        return false;
                }
            } else if (track->instrument_type == FM_SYNTH) {
                FMSynthParameters *p = (FMSynthParameters *) step->data->instrument_data;
                if (p) {
                    if (fwrite(p, sizeof(FMSynthParameters), 1, f) != 1)
                        return false;
                } else {
                    FMSynthParameters dp = defaultFMSynthParameters();
                    if (fwrite(&dp, sizeof(FMSynthParameters), 1, f) != 1)
                        return false;
                }
            } else if (track->instrument_type == NOISE_SYNTH) {
                NoiseSynthParameters *p = (NoiseSynthParameters *) step->data->instrument_data;
                if (p) {
                    if (fwrite(p, sizeof(NoiseSynthParameters), 1, f) != 1)
                        return false;
                } else {
                    NoiseSynthParameters dp = defaultNoiseSynthParameters();
                    if (fwrite(&dp, sizeof(NoiseSynthParameters), 1, f) != 1)
                        return false;
                }
            }
        }
    }
    return true;
}

// Helper to read a track from an open file stream
static bool loadTrackFromStream(FILE *f, Track *track) {
    if (!f || !track)
        return false;

    // Read track-level settings
    if (fread(&track->volume, sizeof(float), 1, f) != 1)
        return false;
    if (fread(&track->pan, sizeof(float), 1, f) != 1)
        return false;

    u8 is_muted, is_soloed;
    if (fread(&is_muted, sizeof(u8), 1, f) != 1)
        return false;
    if (fread(&is_soloed, sizeof(u8), 1, f) != 1)
        return false;
    track->is_muted  = (is_muted != 0);
    track->is_soloed = (is_soloed != 0);

    int filter_type;
    if (fread(&filter_type, sizeof(int), 1, f) != 1)
        return false;
    track->filter.filter_type = (NdspFilterType) filter_type;
    if (fread(&track->filter.cutoff_freq, sizeof(float), 1, f) != 1)
        return false;
    track->filter.update_params = true;

    int inst_type;
    if (fread(&inst_type, sizeof(int), 1, f) != 1)
        return false;
    track->instrument_type = (InstrumentType) inst_type;

    // Read sequencer details
    int n_beats, steps_per_beat;
    if (fread(&n_beats, sizeof(int), 1, f) != 1)
        return false;
    if (fread(&steps_per_beat, sizeof(int), 1, f) != 1)
        return false;

    if (n_beats == 0 && steps_per_beat == 0) {
        return true;
    }

    if (track->sequencer) {
        track->sequencer->n_beats        = n_beats;
        track->sequencer->steps_per_beat = steps_per_beat;
        track->sequencer->cur_step       = 0;
    }

    int num_steps = n_beats * steps_per_beat;
    for (int i = 0; i < num_steps; i++) {
        u8 active;
        if (fread(&active, sizeof(u8), 1, f) != 1)
            return false;

        bool skip_read = (track->sequencer == NULL ||
                          i >= (track->sequencer->n_beats * track->sequencer->steps_per_beat));

        float step_vol, step_pan, step_cutoff;
        int   step_filter_type;
        u8    step_muted, step_soloed;

        if (fread(&step_vol, sizeof(float), 1, f) != 1)
            return false;
        if (fread(&step_pan, sizeof(float), 1, f) != 1)
            return false;
        if (fread(&step_cutoff, sizeof(float), 1, f) != 1)
            return false;
        if (fread(&step_filter_type, sizeof(int), 1, f) != 1)
            return false;
        if (fread(&step_muted, sizeof(u8), 1, f) != 1)
            return false;
        if (fread(&step_soloed, sizeof(u8), 1, f) != 1)
            return false;

        SubSynthParameters    subsynth_p;
        OpusSamplerParameters sampler_p;
        FMSynthParameters     fm_p;
        NoiseSynthParameters  noise_p;

        if (track->instrument_type == SUB_SYNTH) {
            if (fread(&subsynth_p, sizeof(SubSynthParameters), 1, f) != 1)
                return false;
        } else if (track->instrument_type == OPUS_SAMPLER) {
            if (fread(&sampler_p, sizeof(OpusSamplerParameters), 1, f) != 1)
                return false;
        } else if (track->instrument_type == FM_SYNTH) {
            if (fread(&fm_p, sizeof(FMSynthParameters), 1, f) != 1)
                return false;
        } else if (track->instrument_type == NOISE_SYNTH) {
            if (fread(&noise_p, sizeof(NoiseSynthParameters), 1, f) != 1)
                return false;
        }

        if (!skip_read && track->sequencer->steps[i].data) {
            SeqStep *step                  = &track->sequencer->steps[i];
            step->active                   = (active != 0);
            step->data->volume             = step_vol;
            step->data->pan                = step_pan;
            step->data->ndsp_filter_cutoff = step_cutoff;
            step->data->ndsp_filter_type   = (NdspFilterType) step_filter_type;
            step->data->is_muted           = (step_muted != 0);
            step->data->is_soloed          = (step_soloed != 0);

            if (track->instrument_type == SUB_SYNTH) {
                SubSynthParameters *dst = (SubSynthParameters *) step->data->instrument_data;
                if (dst)
                    *dst = subsynth_p;
            } else if (track->instrument_type == OPUS_SAMPLER) {
                OpusSamplerParameters *dst = (OpusSamplerParameters *) step->data->instrument_data;
                if (dst)
                    *dst = sampler_p;
            } else if (track->instrument_type == FM_SYNTH) {
                FMSynthParameters *dst = (FMSynthParameters *) step->data->instrument_data;
                if (dst)
                    *dst = fm_p;
            } else if (track->instrument_type == NOISE_SYNTH) {
                NoiseSynthParameters *dst = (NoiseSynthParameters *) step->data->instrument_data;
                if (dst)
                    *dst = noise_p;
            }
        }
    }

    updateTrackParameters(track, track->default_parameters);
    return true;
}

bool saveTrackToFile(const char *name, int track_idx, Track *tracks, bool create_project,
                     Clock *clock, SampleBank *sample_bank) {
    char filepath[256];
    snprintf(filepath, sizeof(filepath), PATH_PREFIX "Soir/tracks/%s.soirt", name);

    FILE *f = fopen(filepath, "wb");
    if (!f)
        return false;

    if (fwrite("SOIRTRAC", 1, 8, f) != 8) {
        fclose(f);
        return false;
    }
    int version = 1;
    if (fwrite(&version, sizeof(int), 1, f) != 1) {
        fclose(f);
        return false;
    }

    bool success = saveTrackToStream(f, &tracks[track_idx]);
    fclose(f);

    if (success && create_project) {
        // Construct a project containing ONLY this track.
        // We do this by temporarily deactivating steps in all other tracks,
        // saving the project under the same name, and then restoring them.
        bool original_active[N_TRACKS][MAXSEQUENCELENGTH];

        for (int t = 0; t < N_TRACKS; t++) {
            if (t == track_idx)
                continue;
            if (tracks[t].sequencer) {
                int num_steps = tracks[t].sequencer->n_beats * tracks[t].sequencer->steps_per_beat;
                for (int s = 0; s < num_steps; s++) {
                    original_active[t][s]                = tracks[t].sequencer->steps[s].active;
                    tracks[t].sequencer->steps[s].active = false;
                }
            }
        }

        // Save the project file
        saveProject(name, tracks, clock, sample_bank);

        // Restore active states
        for (int t = 0; t < N_TRACKS; t++) {
            if (t == track_idx)
                continue;
            if (tracks[t].sequencer) {
                int num_steps = tracks[t].sequencer->n_beats * tracks[t].sequencer->steps_per_beat;
                for (int s = 0; s < num_steps; s++) {
                    tracks[t].sequencer->steps[s].active = original_active[t][s];
                }
            }
        }
    }

    return success;
}

bool loadTrackFromFile(const char *name, Track *track) {
    char filepath[256];
    snprintf(filepath, sizeof(filepath), PATH_PREFIX "Soir/tracks/%s.soirt", name);

    FILE *f = fopen(filepath, "rb");
    if (!f)
        return false;

    char header[8];
    if (fread(header, 1, 8, f) != 8 || memcmp(header, "SOIRTRAC", 8) != 0) {
        fclose(f);
        return false;
    }
    int version;
    if (fread(&version, sizeof(int), 1, f) != 1 || version != 1) {
        fclose(f);
        return false;
    }

    bool success = loadTrackFromStream(f, track);
    fclose(f);
    return success;
}

bool saveSequenceToFile(const char *name, Track *tracks, Clock *clock) {
    char filepath[256];
    snprintf(filepath, sizeof(filepath), PATH_PREFIX "Soir/sequences/%s.soirp", name);

    FILE *f = fopen(filepath, "wb");
    if (!f)
        return false;

    if (fwrite("SOIRSEQU", 1, 8, f) != 8) {
        fclose(f);
        return false;
    }
    int version = 1;
    if (fwrite(&version, sizeof(int), 1, f) != 1) {
        fclose(f);
        return false;
    }

    // Clock settings
    if (fwrite(&clock->bpm, sizeof(float), 1, f) != 1) {
        fclose(f);
        return false;
    }
    int beats_per_bar = clock->barBeats ? clock->barBeats->beats_per_bar : 4;
    if (fwrite(&beats_per_bar, sizeof(int), 1, f) != 1) {
        fclose(f);
        return false;
    }

    // 5 Tracks
    for (int i = 0; i < N_TRACKS; i++) {
        if (!saveTrackToStream(f, &tracks[i])) {
            fclose(f);
            return false;
        }
    }

    fclose(f);
    return true;
}

bool loadSequenceFromFile(const char *name, Track *tracks, Clock *clock) {
    char filepath[256];
    snprintf(filepath, sizeof(filepath), PATH_PREFIX "Soir/sequences/%s.soirp", name);

    FILE *f = fopen(filepath, "rb");
    if (!f)
        return false;

    char header[8];
    if (fread(header, 1, 8, f) != 8 || memcmp(header, "SOIRSEQU", 8) != 0) {
        fclose(f);
        return false;
    }
    int version;
    if (fread(&version, sizeof(int), 1, f) != 1 || version != 1) {
        fclose(f);
        return false;
    }

    // Clock settings
    float bpm;
    int   beats_per_bar;
    if (fread(&bpm, sizeof(float), 1, f) != 1) {
        fclose(f);
        return false;
    }
    if (fread(&beats_per_bar, sizeof(int), 1, f) != 1) {
        fclose(f);
        return false;
    }
    setBpm(clock, bpm);
    setBeatsPerBar(clock, beats_per_bar);

    // 5 Tracks
    for (int i = 0; i < N_TRACKS; i++) {
        if (!loadTrackFromStream(f, &tracks[i])) {
            fclose(f);
            return false;
        }
    }

    fclose(f);
    return true;
}

bool saveProject(const char *name, Track *tracks, Clock *clock, SampleBank *sample_bank) {
    char filepath[256];
    snprintf(filepath, sizeof(filepath), PATH_PREFIX "Soir/projects/%s.soir", name);

    FILE *f = fopen(filepath, "wb");
    if (!f)
        return false;

    if (fwrite("SOIRPROJ", 1, 8, f) != 8) {
        fclose(f);
        return false;
    }
    int version = 1;
    if (fwrite(&version, sizeof(int), 1, f) != 1) {
        fclose(f);
        return false;
    }

    // Clock
    if (fwrite(&clock->bpm, sizeof(float), 1, f) != 1) {
        fclose(f);
        return false;
    }
    int beats_per_bar = clock->barBeats ? clock->barBeats->beats_per_bar : 4;
    if (fwrite(&beats_per_bar, sizeof(int), 1, f) != 1) {
        fclose(f);
        return false;
    }

    // Sample Bank Loaded Paths
    for (int i = 0; i < MAX_SAMPLES; i++) {
        Sample *sample    = SampleBankGetSample(sample_bank, i);
        u8      is_loaded = (sample != NULL && sample->path != NULL) ? 1 : 0;
        if (fwrite(&is_loaded, sizeof(u8), 1, f) != 1) {
            fclose(f);
            return false;
        }
        if (is_loaded) {
            char path_buf[512];
            memset(path_buf, 0, sizeof(path_buf));
            strncpy(path_buf, sample->path, sizeof(path_buf) - 1);
            if (fwrite(path_buf, sizeof(path_buf), 1, f) != 1) {
                fclose(f);
                return false;
            }
        }
    }

    // 5 Tracks
    for (int i = 0; i < N_TRACKS; i++) {
        if (!saveTrackToStream(f, &tracks[i])) {
            fclose(f);
            return false;
        }
    }

    fclose(f);
    return true;
}

bool loadProject(const char *name, Track *tracks, Clock *clock, SampleBank *sample_bank) {
    char filepath[256];
    snprintf(filepath, sizeof(filepath), PATH_PREFIX "Soir/projects/%s.soir", name);

    FILE *f = fopen(filepath, "rb");
    if (!f)
        return false;

    char header[8];
    if (fread(header, 1, 8, f) != 8 || memcmp(header, "SOIRPROJ", 8) != 0) {
        fclose(f);
        return false;
    }
    int version;
    if (fread(&version, sizeof(int), 1, f) != 1 || version != 1) {
        fclose(f);
        return false;
    }

    // Clock
    float bpm;
    int   beats_per_bar;
    if (fread(&bpm, sizeof(float), 1, f) != 1) {
        fclose(f);
        return false;
    }
    if (fread(&beats_per_bar, sizeof(int), 1, f) != 1) {
        fclose(f);
        return false;
    }
    setBpm(clock, bpm);
    setBeatsPerBar(clock, beats_per_bar);

    // Sample Bank Loaded Paths
    for (int i = 0; i < MAX_SAMPLES; i++) {
        u8 is_loaded;
        if (fread(&is_loaded, sizeof(u8), 1, f) != 1) {
            fclose(f);
            return false;
        }
        if (is_loaded) {
            char path_buf[512];
            if (fread(path_buf, sizeof(path_buf), 1, f) != 1) {
                fclose(f);
                return false;
            }

            LightLock_Lock(&sample_bank->lock);
            if (sample_bank->samples[i] != NULL) {
                sample_dec_ref_main_thread(sample_bank->samples[i]);
                sample_bank->samples[i] = NULL;
            }
            LightLock_Unlock(&sample_bank->lock);

            SampleBankLoadSample(sample_bank, i, path_buf);
        } else {
            LightLock_Lock(&sample_bank->lock);
            if (sample_bank->samples[i] != NULL) {
                sample_dec_ref_main_thread(sample_bank->samples[i]);
                sample_bank->samples[i] = NULL;
            }
            LightLock_Unlock(&sample_bank->lock);
        }
    }

    // 5 Tracks
    for (int i = 0; i < N_TRACKS; i++) {
        if (!loadTrackFromStream(f, &tracks[i])) {
            fclose(f);
            return false;
        }
    }

    fclose(f);
    return true;
}

int getProjectList(const char *dirpath, const char *ext,
                   char list[MAX_PROJECTS][MAX_PROJECT_NAME_LEN]) {
    DIR *dir = opendir(dirpath);
    if (!dir)
        return 0;

    int            count = 0;
    struct dirent *entry;
    size_t         ext_len = strlen(ext);

    while ((entry = readdir(dir)) != NULL && count < MAX_PROJECTS) {
        size_t len = strlen(entry->d_name);
        if (len > ext_len && strcmp(entry->d_name + len - ext_len, ext) == 0) {
            size_t name_len = len - ext_len;
            if (name_len >= MAX_PROJECT_NAME_LEN)
                name_len = MAX_PROJECT_NAME_LEN - 1;
            memcpy(list[count], entry->d_name, name_len);
            list[count][name_len] = '\0';
            count++;
        }
    }

    closedir(dir);
    return count;
}

void createProjectDirs(void) {
    mkdir(PATH_PREFIX "Soir", 0777);
    mkdir(PATH_PREFIX "Soir/projects", 0777);
    mkdir(PATH_PREFIX "Soir/sequences", 0777);
    mkdir(PATH_PREFIX "Soir/tracks", 0777);
}
