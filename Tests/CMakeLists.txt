cmake_minimum_required(VERSION 3.22)

# ============================================================
# Catch2 (v3) via FetchContent
# ============================================================
include(FetchContent)
FetchContent_Declare(
  catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.6.0
)
FetchContent_MakeAvailable(catch2)

# ============================================================
# Gather all test sources recursively
# ============================================================
file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/**/test_*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"   # ensure our custom main is built
)

add_executable(MIDIControl001_tests ${TEST_SOURCES})

# ============================================================
# Include plugin and JUCE source paths
# ============================================================
target_include_directories(MIDIControl001_tests PRIVATE
  ${PROJECT_SOURCE_DIR}/Source
  ${CMAKE_BINARY_DIR}/MIDIControl001_artefacts/JuceLibraryCode
)

# ============================================================
# Optional: directly compile small plugin modules needed for state tests
# ============================================================
target_sources(MIDIControl001_tests PRIVATE
  ${PROJECT_SOURCE_DIR}/Source/params/ParamLayout.cpp
  ${PROJECT_SOURCE_DIR}/Source/plugin/PluginProcessor.cpp
  ${PROJECT_SOURCE_DIR}/Source/dsp/envelopes/EnvelopeA.cpp
)

# ============================================================
# Compile definitions consistent with plugin target
# ============================================================
target_compile_definitions(MIDIControl001_tests PRIVATE
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0
  JUCE_STANDALONE_APPLICATION=1
)

# ============================================================
# Link libraries
# ============================================================
target_link_libraries(MIDIControl001_tests PRIVATE
  Catch2::Catch2
  juce::juce_audio_utils
  juce::juce_audio_processors
  juce::juce_audio_basics
  juce::juce_gui_basics
  juce::juce_dsp
)

# ============================================================
# Ensure plugin target (which generates JuceHeader.h) is built first
# ============================================================
add_dependencies(MIDIControl001_tests MIDIControl001)

# ============================================================
# Register with CTest
# ============================================================
include(Catch)
catch_discover_tests(MIDIControl001_tests)
