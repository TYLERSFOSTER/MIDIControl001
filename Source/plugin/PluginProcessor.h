// PluginProcessor.h
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "params/ParamLayout.h"
#include "params/ParameterIDs.h"

// NEW:
#include "dsp/VoiceManager.h"
#include "params/ParameterSnapshot.h"

class MIDIControl001AudioProcessor : public juce::AudioProcessor
{
public:
    MIDIControl001AudioProcessor();
    ~MIDIControl001AudioProcessor() override = default;

    // ============================================================
    // JUCE AudioProcessor overrides
    // ============================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "MIDIControl001"; }

    double getTailLengthSeconds() const override { return 0.0; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    // ============================================================
    // Phase III – B5: expose factory injection for tests / tooling
    // ============================================================
    void setVoiceFactory(VoiceManager::VoiceFactory factory)
    {
        voiceManager_.setVoiceFactory(std::move(factory));
    }

    // ============================================================
    // Phase IV A11-1 — allow external toggle (GUI later)
    // ============================================================
    void setAudioSynthesisEnabled(bool enabled)
    {
        voiceManager_.setAudioSynthesisEnabled(enabled);
    }

    // ============================================================
    // Public Members
    // ============================================================
    juce::AudioProcessorValueTreeState apvts;

private:
    // ===== NEW: voice engine and scratch mono buffer =====
    VoiceManager voiceManager_;
    juce::AudioBuffer<float> monoScratch_;
    double sampleRate_ = 44100.0;

    // Small helper to read a snapshot from APVTS
    ParameterSnapshot makeSnapshotFromParams() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MIDIControl001AudioProcessor)
};

// Factory function required by JUCE
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();
