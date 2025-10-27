#pragma once
#include <JuceHeader.h>

class MIDIControl001AudioProcessor : public juce::AudioProcessor
{
public:
    MIDIControl001AudioProcessor();
    ~MIDIControl001AudioProcessor() override = default;

    // Core AudioProcessor overrides
    void prepareToPlay (double sampleRate, int samplesPerBlock) override {}
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}

    // Plugin description
    const juce::String getName() const override { return "MIDIControl001"; }

    // Required pure virtuals
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override {}
    void setStateInformation (const void* data, int sizeInBytes) override {}

    // Editor factory
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
};
