cmake_minimum_required(VERSION 3.22)

include(FetchContent)

# ============================================================
# Catch2 + nlohmann_json
# ============================================================
FetchContent_Declare(
  catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.6.0
)
FetchContent_MakeAvailable(catch2)

FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

# ============================================================
# Gather test sources
# ============================================================
file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/**/test_*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
)

add_executable(MIDIControl001_tests ${TEST_SOURCES})

target_include_directories(MIDIControl001_tests PRIVATE
  ${PROJECT_SOURCE_DIR}/Source
  ${PROJECT_SOURCE_DIR}/Tests
  ${PROJECT_SOURCE_DIR}/Tests/utils
  ${CMAKE_BINARY_DIR}/MIDIControl001_artefacts/JuceLibraryCode
)

# ============================================================
# Link directly against the shared plugin code
# ============================================================
set(SHARED_LIB "${CMAKE_BINARY_DIR}/MIDIControl001_artefacts/Debug/libMIDIControl001_SharedCode.a")

if (EXISTS "${SHARED_LIB}")
  message(STATUS "Linking tests with shared plugin library: ${SHARED_LIB}")
  target_link_libraries(MIDIControl001_tests PRIVATE "${SHARED_LIB}")
else()
  message(WARNING "Shared plugin library not found yet; will fall back to adding source files.")
  target_sources(MIDIControl001_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/Source/plugin/PluginProcessor.cpp
    ${PROJECT_SOURCE_DIR}/Source/plugin/PluginProcessor.h
  )
endif()

# ============================================================
# Additional core sources for tests
# ============================================================
target_sources(MIDIControl001_tests PRIVATE
  ${PROJECT_SOURCE_DIR}/Source/params/ParamLayout.cpp
  ${PROJECT_SOURCE_DIR}/Source/dsp/envelopes/EnvelopeA.cpp
  ${PROJECT_SOURCE_DIR}/Source/dsp/voices/VoiceA.cpp
)

target_compile_definitions(MIDIControl001_tests PRIVATE
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0
  JUCE_STANDALONE_APPLICATION=1
  JUCE_UNIT_TESTS=1
)

target_link_libraries(MIDIControl001_tests PRIVATE
  Catch2::Catch2
  nlohmann_json::nlohmann_json
  juce::juce_audio_utils
  juce::juce_audio_processors
  juce::juce_audio_basics
  juce::juce_gui_basics
  juce::juce_dsp
)

add_dependencies(MIDIControl001_tests MIDIControl001)

include(Catch)
catch_discover_tests(MIDIControl001_tests)

# ============================================================
# Post-build diagnostics hook (Phase 4-D)
# ============================================================
add_custom_command(
  TARGET MIDIControl001_tests
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E echo "Running post-build report..."
  COMMAND bash ${CMAKE_SOURCE_DIR}/tools/post_build_report.sh
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
