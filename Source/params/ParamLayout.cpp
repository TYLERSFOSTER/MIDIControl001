#include <juce_audio_processors/juce_audio_processors.h>
#include "ParameterIDs.h"

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using namespace juce;

    DBG("=== Building ParameterLayout (diagnostic multi-voice) ===");

    AudioProcessorValueTreeState::ParameterLayout layout;

    // ============================================================
    // Master-level parameters
    // ============================================================
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

    // ============================================================
    // Global voice-mode selector (Phase III B3)
    // ------------------------------------------------------------
    // IMPORTANT: The order here must match the VoiceMode enum
    // (typically defined in ParameterSnapshot.h):
    //
    //   0 -> VoiceA
    //   1 -> VoiceDopp
    //   2 -> VoiceLET
    //   3 -> VoiceFM
    //
    // For now, the engine still instantiates only VoiceA, so
    // changing this in the UI has *no* effect on DSP output yet.
    // ============================================================
    {
        StringArray modeNames;
        modeNames.add("VoiceA");    // index 0 -> VoiceMode::VoiceA
        modeNames.add("VoiceDopp"); // index 1 -> VoiceMode::VoiceDopp
        modeNames.add("VoiceLET");  // index 2 -> VoiceMode::VoiceLET
        modeNames.add("VoiceFM");   // index 3 -> VoiceMode::VoiceFM

        layout.add(std::make_unique<AudioParameterChoice>(
            ParameterIDs::voiceMode,
            "Voice Mode",
            modeNames,
            0 // default index: VoiceA
        ));
    }

    // ============================================================
    // Global (non-per-voice) DSP parameters
    // ============================================================
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
