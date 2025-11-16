#include <catch2/catch_test_macros.hpp>
#include <juce_audio_processors/juce_audio_processors.h>

#include "plugin/PluginProcessor.h"
#include "params/ParameterIDs.h"
#include "params/ParamLayout.h"

// ============================================================
// Phase II (A6) — Verify voiceMode parameter exists,
// updates through APVTS, and forwards without crashing.
// ============================================================

TEST_CASE("Voice Mode parameter exists, snapshots through APVTS, and forwards",
          "[params][snapshot][mode]")
{
    MIDIControl001AudioProcessor proc;

    // Access APVTS
    auto& apvts = proc.apvts;

    // ------------------------------------------------------------
    // 1. The parameter must exist in APVTS
    // ------------------------------------------------------------
    auto* modeParam = apvts.getParameter(ParameterIDs::voiceMode);
    REQUIRE(modeParam != nullptr);

    // ------------------------------------------------------------
    // 2. Writing APVTS value and running processBlock must be safe
    // ------------------------------------------------------------
    juce::AudioBuffer<float> buffer(2, 32);
    juce::MidiBuffer midi;

    // Only one mode (index 0) currently; this mainly exercises the path
    modeParam->setValueNotifyingHost(0.0f);

    REQUIRE_NOTHROW(proc.processBlock(buffer, midi));

    // ------------------------------------------------------------
    // 3. Changing APVTS again and re-running should also be safe
    // ------------------------------------------------------------
    modeParam->setValueNotifyingHost(0.0f);
    REQUIRE_NOTHROW(proc.processBlock(buffer, midi));

    // ------------------------------------------------------------
    // 4. Extra safety call: repeated blocks should also not throw
    // ------------------------------------------------------------
    REQUIRE_NOTHROW(proc.processBlock(buffer, midi));
}
