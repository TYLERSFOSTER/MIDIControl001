#include <juce_audio_processors/juce_audio_processors.h>
#include "ParameterIDs.h"

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using namespace juce;

    DBG("=== Building ParameterLayout (diagnostic single-voice) ===");

    AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<AudioParameterFloat>(
        ParameterIDs::masterVolume,
        "Master Volume",
        NormalisableRange<float>(-60.0f, 0.0f),
        -6.0f));

    layout.add(std::make_unique<AudioParameterFloat>(
        ParameterIDs::masterMix,
        "Master Mix",
        NormalisableRange<float>(0.0f, 1.0f),
        1.0f));

    layout.add(std::make_unique<AudioParameterChoice>(
        ParameterIDs::voiceMode,
        "Voice Mode",
        StringArray{ "voiceA" }, // Phase II Step A: only VoiceA exists
        0 // default index: "voiceA"
    ));

    layout.add(std::make_unique<AudioParameterFloat>(
        ParameterIDs::oscFreq,
        "Osc Frequency",
        NormalisableRange<float>(20.0f, 20000.0f, 0.01f, 0.3f),
        440.0f));

    layout.add(std::make_unique<AudioParameterFloat>(
        ParameterIDs::envAttack,
        "Env Attack",
        NormalisableRange<float>(0.001f, 2.0f),
        0.01f));

    layout.add(std::make_unique<AudioParameterFloat>(
        ParameterIDs::envRelease,
        "Env Release",
        NormalisableRange<float>(0.01f, 5.0f),
        0.2f));

    DBG("=== Done Building ParameterLayout ===");
    return layout;
}
