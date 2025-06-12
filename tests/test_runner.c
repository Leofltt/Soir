#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

// Declare test functions
extern void test_sequence_length_update(void);
extern void test_sequence_step_update(void);
extern void test_envelope_initialization(void);

int main(void) {
    UNITY_BEGIN();
    
    // Sequencer tests
    RUN_TEST(test_sequence_length_update);
    RUN_TEST(test_sequence_step_update);
    
    // Envelope tests
    RUN_TEST(test_envelope_initialization);
    
    return UNITY_END();
}