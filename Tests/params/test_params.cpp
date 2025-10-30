// Tests/test_params.cpp
#include <catch2/catch_test_macros.hpp>
#include <juce_audio_processors/juce_audio_processors.h>
#include "params/ParamLayout.h"
#include "params/ParameterIDs.h"

struct MinimalProcessor : juce::AudioProcessor
{
    MinimalProcessor() : juce::AudioProcessor (BusesProperties()) {}

    // --- required overrides (bare-minimum) ---
    const juce::String getName() const override { return "Minimal"; }
    void prepareToPlay (double, int) override {}
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override { return true; }
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
    double getTailLengthSeconds() const override { return 0.0; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override {}
    void setStateInformation (const void*, int) override {}
};

TEST_CASE("APVTS layout contains master volume")
{
    auto layout = createParameterLayout();

    MinimalProcessor proc;
    juce::AudioProcessorValueTreeState apvts (proc, nullptr, "State", std::move(layout));

    REQUIRE(apvts.getParameter(ParameterIDs::masterVolume) != nullptr);
}
