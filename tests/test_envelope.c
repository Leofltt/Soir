#include "mock_3ds.h"
#include "envelope.h"
#include "unity.h"

#define SAMPLE_RATE 44100.0f

void test_envelope_initialization(void) {
    Envelope env = defaultEnvelopeStruct(SAMPLE_RATE);
    TEST_ASSERT_EQUAL(ENVELOPE_STATE_IDLE, env.state);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, env.output);
}

void test_envelope_trigger_and_release(void) {
    Envelope env = defaultEnvelopeStruct(SAMPLE_RATE);

    // Triggering from IDLE should go to ATTACK
    triggerEnvelope(&env);
    TEST_ASSERT_EQUAL(ENVELOPE_STATE_ATTACK, env.state);

    // Releasing from ATTACK should go to RELEASE
    releaseEnvelope(&env);
    TEST_ASSERT_EQUAL(ENVELOPE_STATE_RELEASE, env.state);

    // Let it go back to IDLE
    env.output = 0.0f;
    env.state  = ENVELOPE_STATE_IDLE;

    // Trigger again
    triggerEnvelope(&env);
    env.state = ENVELOPE_STATE_DECAY; // Manually set for testing
    releaseEnvelope(&env);
    TEST_ASSERT_EQUAL(ENVELOPE_STATE_RELEASE, env.state);

    env.state = ENVELOPE_STATE_SUSTAIN; // Manually set for testing
    releaseEnvelope(&env);
    TEST_ASSERT_EQUAL(ENVELOPE_STATE_RELEASE, env.state);
}

void test_envelope_adsr_progression(void) {
    Envelope env = defaultEnvelopeStruct(SAMPLE_RATE);
    updateEnvelope(&env, 10, 10, 0.5f, 10, 100); // 10ms A, 10ms D, 0.5 sus, 10ms R

    // --- Test Attack ---
    triggerEnvelope(&env);
    TEST_ASSERT_EQUAL(ENVELOPE_STATE_ATTACK, env.state);
    // Simulate ~10ms of samples to complete attack
    for (int i = 0; i < (int) (SAMPLE_RATE * 0.010); i++) {
        nextEnvelopeSample(&env);
    }
    TEST_ASSERT_EQUAL(ENVELOPE_STATE_DECAY, env.state);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, env.output);

    // --- Test Decay ---
    // Simulate ~10ms of decay to complete decay
    for (int i = 0; i < (int) (SAMPLE_RATE * 0.010); i++) {
        nextEnvelopeSample(&env);
    }
    TEST_ASSERT_EQUAL(ENVELOPE_STATE_SUSTAIN, env.state);
    TEST_ASSERT_EQUAL_FLOAT(0.5f, env.output);

    // --- Test Sustain ---
    // In sustain, output should not change
    for (int i = 0; i < 100; i++) {
        nextEnvelopeSample(&env);
    }
    TEST_ASSERT_EQUAL(ENVELOPE_STATE_SUSTAIN, env.state);
    TEST_ASSERT_EQUAL_FLOAT(0.5f, env.output);

    // --- Test Release ---
    releaseEnvelope(&env);
    TEST_ASSERT_EQUAL(ENVELOPE_STATE_RELEASE, env.state);
    // Simulate ~10ms of release
    for (int i = 0; i < (int) (SAMPLE_RATE * 0.010); i++) {
        nextEnvelopeSample(&env);
    }
    TEST_ASSERT_EQUAL(ENVELOPE_STATE_IDLE, env.state);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, env.output);
}