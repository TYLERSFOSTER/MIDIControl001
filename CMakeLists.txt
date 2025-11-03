cmake_minimum_required(VERSION 3.20)  # 3.20 is enough for JUCE 7
project(MIDIControl001 VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

cmake_policy(SET CMP0175 OLD)

include(FetchContent)

# ============================================================
# JUCE FetchContent (using stable JUCE 7.0.9)
# ============================================================
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        7.0.9
)
FetchContent_MakeAvailable(juce)

# ============================================================
# Plugin target
# ============================================================
juce_add_plugin(MIDIControl001
    COMPANY_NAME "YourCompany"
    BUNDLE_ID    com.yourcompany.midicontrol001
    IS_SYNTH     TRUE
    NEEDS_MIDI_INPUT TRUE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE YCmp
    PLUGIN_CODE Mc01
    FORMATS VST3 Standalone
    PRODUCT_NAME "MIDIControl001"

    SOURCES
        Source/plugin/PluginProcessor.cpp
        Source/plugin/PluginProcessor.h
        Source/plugin/PluginEditor.cpp
        Source/plugin/PluginEditor.h

        Source/params/ParamLayout.cpp
        Source/params/ParamLayout.h
        Source/params/ParameterIDs.h

        Source/dsp/VoiceManager.h
        Source/dsp/voices/VoiceA.h
        Source/dsp/voices/VoiceA.cpp
        Source/dsp/oscillators/OscillatorA.h
        Source/dsp/envelopes/EnvelopeA.h
        Source/dsp/envelopes/EnvelopeA.cpp

        Source/gui/VoiceGUI.h
        Source/gui/MasterGUI.h

        Source/utils/Logger.h
        Source/utils/MathHelpers.h
)

target_sources(MIDIControl001
    PRIVATE
        # plugin
        Source/plugin/PluginProcessor.cpp
        Source/plugin/PluginEditor.cpp
        Source/plugin/PluginProcessor.h
        Source/plugin/PluginEditor.h

        # params
        Source/params/ParamLayout.cpp
        Source/params/ParamLayout.h
        Source/params/ParameterIDs.h

        # dsp core
        Source/dsp/VoiceManager.h
        Source/dsp/oscillators/OscillatorA.h
        Source/dsp/envelopes/EnvelopeA.h
        Source/dsp/envelopes/EnvelopeA.cpp
        Source/dsp/voices/VoiceA.cpp

        # gui
        Source/gui/VoiceGUI.h
        Source/gui/MasterGUI.h

        # utils
        Source/utils/Logger.h
        Source/utils/MathHelpers.h
)

juce_generate_juce_header(MIDIControl001)

target_link_libraries(MIDIControl001
    PRIVATE
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_audio_basics
        juce::juce_gui_basics
        juce::juce_dsp
)

# Make headers under Source/ available as "plugin/...", "params/...", etc.
target_include_directories(MIDIControl001
    PRIVATE
        ${PROJECT_SOURCE_DIR}/Source
)

# ============================================================
# Recommended plugin compile definitions
# ============================================================
target_compile_definitions(MIDIControl001
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

# ============================================================
# Tests
# ============================================================
enable_testing()
add_subdirectory(Tests)
