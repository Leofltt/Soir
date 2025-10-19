cmake_minimum_required(VERSION 3.15)
project(SoirTests)

set(CMAKE_C_STANDARD 11)

# Add Unity test framework
add_library(unity STATIC 
    unity/unity.c
    unity/unity_internals.h
    unity/unity.h
)

# Create test executable
add_executable(test_runner
    test_sequencer.c
    test_track_parameters.c
    test_envelope.c
    ../source/sequencer.c
    ../source/track_parameters.c
    ../source/envelope.c
)

target_include_directories(test_runner PRIVATE 
    unity
    ../include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Define TESTING to exclude 3DS-specific code
target_compile_definitions(test_runner PRIVATE TESTING)