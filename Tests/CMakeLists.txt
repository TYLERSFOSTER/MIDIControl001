cmake_minimum_required(VERSION 3.22)

# Catch2 (v3) via FetchContent
include(FetchContent)
FetchContent_Declare(
  catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.6.0
)
FetchContent_MakeAvailable(catch2)

# Build a small test binary
add_executable(MIDIControl001_tests
  test_params.cpp
  test_dsp.cpp
  test_processor_smoke.cpp
)

# Include plugin sources you want to exercise (headers are fine; add .cpp only if needed)
target_sources(MIDIControl001_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/Source/params/ParamLayout.cpp
)

target_include_directories(MIDIControl001_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/Source
)

# Link JUCE bits required by what you test
target_link_libraries(MIDIControl001_tests PRIVATE
  Catch2::Catch2WithMain
  juce::juce_audio_processors
  juce::juce_audio_basics
  juce::juce_dsp
)

# Register with CTest
include(Catch)
catch_discover_tests(MIDIControl001_tests)
