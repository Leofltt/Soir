#include "unity.h"
#include "project.h"
#include "track.h"
#include "clock.h"
#include "engine_constants.h"
#include <stdio.h>
#include <string.h>

void test_track_save_and_load(void) {
    Track track;
    memset(&track, 0, sizeof(Track));
    track.volume = 0.75f;
    track.pan = -0.5f;
    track.is_muted = true;
    track.is_soloed = false;
    track.filter.filter_type = NDSP_BIQUAD_LOPASS;
    track.filter.cutoff_freq = 1200.0f;
    track.instrument_type = SUB_SYNTH;

    // Create a mock sequencer
    Sequencer seq;
    memset(&seq, 0, sizeof(Sequencer));
    seq.n_beats = 4;
    seq.steps_per_beat = 4;
    
    SeqStep steps[16];
    TrackParameters params[16];
    SubSynthParameters synth_params[16];
    
    for (int i = 0; i < 16; i++) {
        params[i] = defaultTrackParameters(0, &synth_params[i]);
        steps[i].active = (i % 2 == 0); // Alternate active steps
        steps[i].data = &params[i];
    }
    seq.steps = steps;
    track.sequencer = &seq;

    // Save track to a temporary file
    const char *test_filename = "test_track_save";
    bool save_success = saveTrackToFile(test_filename, 0, &track, false, NULL, NULL);
    TEST_ASSERT_TRUE(save_success);

    // Reset track values
    track.volume = 0.0f;
    track.pan = 0.0f;
    track.is_muted = false;
    track.filter.cutoff_freq = 0.0f;
    for (int i = 0; i < 16; i++) {
        steps[i].active = false;
    }

    // Load track back
    bool load_success = loadTrackFromFile(test_filename, &track);
    TEST_ASSERT_TRUE(load_success);

    // Assert values were restored correctly
    TEST_ASSERT_EQUAL_FLOAT(0.75f, track.volume);
    TEST_ASSERT_EQUAL_FLOAT(-0.5f, track.pan);
    TEST_ASSERT_TRUE(track.is_muted);
    TEST_ASSERT_EQUAL(NDSP_BIQUAD_LOPASS, track.filter.filter_type);
    TEST_ASSERT_EQUAL_FLOAT(1200.0f, track.filter.cutoff_freq);
    
    for (int i = 0; i < 16; i++) {
        TEST_ASSERT_EQUAL(i % 2 == 0, steps[i].active);
    }

    // Clean up temporary file
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "./Soir/tracks/%s.soirt", test_filename);
    remove(filepath);
}

void test_sequence_save_and_load(void) {
    Track tracks[N_TRACKS];
    memset(tracks, 0, sizeof(tracks));
    
    Clock clock;
    MusicalTime mt = { .beats_per_bar = 4 };
    clock.bpm = 135.0f;
    clock.barBeats = &mt;

    // Setup track sequencers
    Sequencer seqs[N_TRACKS];
    SeqStep steps[N_TRACKS][16];
    TrackParameters params[N_TRACKS][16];
    SubSynthParameters synth_params[N_TRACKS][16];

    for (int t = 0; t < N_TRACKS; t++) {
        tracks[t].instrument_type = SUB_SYNTH;
        tracks[t].volume = 0.8f;
        tracks[t].pan = 0.1f * t;
        
        memset(&seqs[t], 0, sizeof(Sequencer));
        seqs[t].n_beats = 4;
        seqs[t].steps_per_beat = 4;
        
        for (int i = 0; i < 16; i++) {
            params[t][i] = defaultTrackParameters(t, &synth_params[t][i]);
            steps[t][i].active = (i == t);
            steps[t][i].data = &params[t][i];
        }
        seqs[t].steps = steps[t];
        tracks[t].sequencer = &seqs[t];
    }

    const char *test_filename = "test_seq_save";
    bool save_success = saveSequenceToFile(test_filename, tracks, &clock);
    TEST_ASSERT_TRUE(save_success);

    // Reset clock and track volume
    clock.bpm = 0.0f;
    for (int t = 0; t < N_TRACKS; t++) {
        tracks[t].volume = 0.0f;
        for (int i = 0; i < 16; i++) {
            steps[t][i].active = false;
        }
    }

    // Load back
    bool load_success = loadSequenceFromFile(test_filename, tracks, &clock);
    TEST_ASSERT_TRUE(load_success);

    // Assert clock
    TEST_ASSERT_EQUAL_FLOAT(135.0f, clock.bpm);
    
    // Assert track values
    for (int t = 0; t < N_TRACKS; t++) {
        TEST_ASSERT_EQUAL_FLOAT(0.8f, tracks[t].volume);
        for (int i = 0; i < 16; i++) {
            TEST_ASSERT_EQUAL(i == t, steps[t][i].active);
        }
    }

    // Clean up temporary file
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "./Soir/sequences/%s.soirp", test_filename);
    remove(filepath);
}
