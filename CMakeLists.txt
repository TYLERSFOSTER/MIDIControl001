cmake_minimum_required(VERSION 3.22)
project(MIDIControl001 VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(juce)

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
        Source/dsp/Voice.h
        Source/dsp/voices/VoiceA.h
        Source/dsp/oscillators/OscillatorA.h
        Source/dsp/envelopes/EnvelopeA.h

        # gui (if separate components exist)
        Source/gui/VoiceGUI.h
        Source/gui/MasterGUI.h

        # utils (optional)
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

# Recommended definitions for plugins
target_compile_definitions(MIDIControl001
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

enable_testing()
add_subdirectory(Tests)

add_dependencies(MIDIControl001 juce::juce_graphics)
add_dependencies(MIDIControl001_tests juce::juce_graphics)

