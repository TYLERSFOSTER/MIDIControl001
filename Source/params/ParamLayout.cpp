#include <juce_audio_processors/juce_audio_processors.h>
#include "ParameterIDs.h"

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> params;

    params.push_back(std::make_unique<AudioParameterFloat>(
        ParameterIDs::masterVolume, "Master Volume",
        NormalisableRange<float>(-60.0f, 0.0f, 0.1f), -6.0f));

    params.push_back(std::make_unique<AudioParameterFloat>(
        ParameterIDs::masterMix, "Master Mix",
        NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.0f));

    const int numVoices = 3;
    for (int i = 0; i < numVoices; ++i)
    {
        juce::String voicePrefix = "voices/voice" + juce::String(i + 1) + "/";

        params.push_back(std::make_unique<AudioParameterChoice>(
            voicePrefix + ParameterIDs::oscType, "Oscillator Type",
            juce::StringArray{ "Sine","Square","Saw","Noise" }, 0));

        params.push_back(std::make_unique<AudioParameterFloat>(
            voicePrefix + ParameterIDs::oscFreq, "Oscillator Frequency",
            NormalisableRange<float>(20.0f, 20000.0f, 0.1f, 0.25f), 440.0f));

        params.push_back(std::make_unique<AudioParameterFloat>(
            voicePrefix + ParameterIDs::envAttack, "Envelope Attack",
            NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.3f), 0.01f));

        params.push_back(std::make_unique<AudioParameterFloat>(
            voicePrefix + ParameterIDs::envRelease, "Envelope Release",
            NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.3f), 0.2f));

        params.push_back(std::make_unique<AudioParameterBool>(
            voicePrefix + ParameterIDs::scopeEnabled, "Scope Enabled", true));

        params.push_back(std::make_unique<AudioParameterFloat>(
            voicePrefix + ParameterIDs::scopeBrightness, "Scope Brightness",
            NormalisableRange<float>(0.1f, 2.0f, 0.01f), 1.0f));
    }

    return { params.begin(), params.end() };
}
