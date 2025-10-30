// Tests/test_processor_smoke.cpp
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <juce_audio_processors/juce_audio_processors.h>

#include "plugin/PluginProcessor.h"
#include "params/ParameterIDs.h"

using Catch::Approx;

// ------------------------------------------------------------
// Helpers
// ------------------------------------------------------------
static std::unique_ptr<MIDIControl001AudioProcessor> makeProc()
{
    auto p = std::make_unique<MIDIControl001AudioProcessor>();
    p->prepareToPlay(48'000.0, 512);
    return p;
}

static float getParam(MIDIControl001AudioProcessor& proc, const juce::String& id)
{
    if (auto* a = proc.apvts.getRawParameterValue(id))
        return a->load(); // atomic<float> -> float
    REQUIRE(false);       // hard fail if missing
    return std::numeric_limits<float>::quiet_NaN();
}

// Write through the ValueTree (simple & state-safe)
static void setParamVT(MIDIControl001AudioProcessor& proc, const juce::String& id, float value)
{
    proc.apvts.getParameterAsValue(id).setValue(value);
}

// Write with host notifications (good for exercising listeners/automation)
static void setParamNotifyingHost(MIDIControl001AudioProcessor& proc, const juce::String& id, float value)
{
    if (auto* p = dynamic_cast<juce::RangedAudioParameter*>(proc.apvts.getParameter(id)))
    {
        const float norm = p->convertTo0to1(value);
        p->beginChangeGesture();
        p->setValueNotifyingHost(norm);
        p->endChangeGesture();
        return;
    }
    REQUIRE(false); // parameter not found -> fail the test
}

static juce::MemoryBlock saveState(MIDIControl001AudioProcessor& proc)
{
    juce::MemoryBlock mb;
    proc.getStateInformation(mb);
    return mb;
}

static void loadState(MIDIControl001AudioProcessor& proc, const juce::MemoryBlock& mb)
{
    proc.setStateInformation(mb.getData(), static_cast<int>(mb.getSize()));
}

// ------------------------------------------------------------
// Tests
// ------------------------------------------------------------

TEST_CASE("APVTS loads default parameter values", "[apvts][defaults]")
{
    auto proc = makeProc();

    // masterVolume default in ParamLayout: -6.0 dB
    const float vol = getParam(*proc, ParameterIDs::masterVolume);
    CHECK(vol == Approx(-6.0f).margin(1e-6f));
}

TEST_CASE("APVTS parameter set/get roundtrip", "[apvts][setget]")
{
    auto proc = makeProc();

    // Use host-notifying path here just to exercise the full param route
    setParamNotifyingHost(*proc, ParameterIDs::masterVolume, -12.0f);
    const float vol = getParam(*proc, ParameterIDs::masterVolume);
    CHECK(vol == Approx(-12.0f).margin(1e-6f));

    // And put it back to default to prove reversibility
    setParamNotifyingHost(*proc, ParameterIDs::masterVolume, -6.0f);
    const float vol2 = getParam(*proc, ParameterIDs::masterVolume);
    CHECK(vol2 == Approx(-6.0f).margin(1e-6f));
}

TEST_CASE("Processor state save/restore preserves parameters", "[apvts][state]")
{
    // ---- mutate before saving
    auto proc1 = makeProc();
    setParamVT(*proc1, ParameterIDs::masterVolume, -12.0f);  // ValueTree path ensures state is updated
    CHECK(getParam(*proc1, ParameterIDs::masterVolume) == Approx(-12.0f).margin(1e-6f));

    // ---- save state
    const auto state = saveState(*proc1);

    // ---- new instance, restore state
    auto proc2 = makeProc();
    // Ensure it's at default prior to load (sanity)
    CHECK(getParam(*proc2, ParameterIDs::masterVolume) == Approx(-6.0f).margin(1e-6f));

    loadState(*proc2, state);

    // ---- verify restored value
    CHECK(getParam(*proc2, ParameterIDs::masterVolume) == Approx(-12.0f).margin(1e-6f));
}
