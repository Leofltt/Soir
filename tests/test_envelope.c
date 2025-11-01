#include "mock_3ds.h"
#include "envelope.h"
#include "unity.h"

void test_envelope_initialization(void) {
    Envelope env = defaultEnvelopeStruct(44100.0f);
    TEST_ASSERT_EQUAL(ENV_OFF, env.gate);
    TEST_ASSERT_EQUAL(0, env.env_pos);
    TEST_ASSERT_NOT_NULL(env.env_buffer);

    if (env.env_buffer) {
        linearFree(env.env_buffer);
    }
}