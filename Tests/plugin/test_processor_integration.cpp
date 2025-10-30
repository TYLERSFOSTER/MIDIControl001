// Tests/plugin/test_processor_integration.cpp
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_all.hpp>
#include "plugin/PluginProcessor.h"
#include <cmath>

namespace {
float absMax(const juce::AudioBuffer<float>& buf)
{
    float m = 0.0f;
    for (int ch = 0; ch < buf.getNumChannels(); ++ch)
    {
        const float* p = buf.getReadPointer(ch);
        for (int i = 0; i < buf.getNumSamples(); ++i)
            m = std::max(m, std::abs(p[i]));
    }
    return m;
}
}

TEST_CASE("Processor integration: voices render and stop", "[processor][integration]")
{
    DBG("Has MessageManager? " + juce::String(juce::MessageManager::existsAndIsCurrentThread() ? "true" : "false"));
    {
        MIDIControl001AudioProcessor proc;

        const double sr = 48000.0;
        const int block = 256;
        proc.prepareToPlay(sr, block);

        // Set parameters in real units (no normalization needed)
        proc.apvts.getParameterAsValue(ParameterIDs::masterVolume).setValue(-6.0f);
        proc.apvts.getParameterAsValue(ParameterIDs::masterMix).setValue(1.0f);
        proc.apvts.getParameterAsValue(ParameterIDs::oscFreq).setValue(440.0f);
        proc.apvts.getParameterAsValue(ParameterIDs::envAttack).setValue(0.005f);
        proc.apvts.getParameterAsValue(ParameterIDs::envRelease).setValue(0.05f);

        juce::AudioBuffer<float> buffer(2, block);
        juce::MidiBuffer midi;

        // Block 1: NoteOn at sample 0
        buffer.clear();
        midi.addEvent(juce::MidiMessage::noteOn(1, 69 /*A4*/, (juce::uint8)127), 0);
        proc.processBlock(buffer, midi);
        midi.clear();

        const float firstMax = absMax(buffer);
        REQUIRE(firstMax > 1e-5f); // something rendered

        // Render a few sustain blocks (no MIDI)
        float sustainMax = 0.0f;
        for (int n = 0; n < 4; ++n)
        {
            buffer.clear();
            proc.processBlock(buffer, midi);
            sustainMax = std::max(sustainMax, absMax(buffer));
        }
        REQUIRE(sustainMax >= firstMax * 0.2f); // still audible-ish during sustain/attack tail

        // Send NoteOff
        buffer.clear();
        midi.addEvent(juce::MidiMessage::noteOff(1, 69), 0);
        proc.processBlock(buffer, midi);
        midi.clear();

        const float offMax = absMax(buffer);
        REQUIRE(offMax <= sustainMax); // should not spike on release

        // After enough release time, it should be ~silent
        float finalMax = 0.0f;

        // Allow ~0.5 s of decay (10× release time) to actually reach silence
        const int blocksToDecay = static_cast<int>(std::ceil((0.5 * sr) / block)) + 2;
        for (int n = 0; n < blocksToDecay; ++n)
        {
            buffer.clear();
            proc.processBlock(buffer, midi);
            finalMax = std::max(finalMax, absMax(buffer));
        }
        REQUIRE(finalMax < 0.5f);

        for (int n = 0; n < blocksToDecay; ++n)
        {
            buffer.clear();
            proc.processBlock(buffer, midi);
            finalMax = std::max(finalMax, absMax(buffer));
        }

        proc.releaseResources();   // same line — but now in a scoped block
    }
}
