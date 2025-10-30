#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "params/ParameterIDs.h"

MIDIControl001AudioProcessor::MIDIControl001AudioProcessor()
  : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    apvts(*this, nullptr, "Parameters", createParameterLayout())
{}

void MIDIControl001AudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    sampleRate_ = sampleRate;
    voiceManager_.prepare(sampleRate_);

    monoScratch_.setSize(1, samplesPerBlock);
    monoScratch_.clear();
}

void MIDIControl001AudioProcessor::releaseResources()
{
    DBG("releaseResources begin");

    voiceManager_ = {};
    monoScratch_.setSize(0, 0);

    if (!juce::MessageManager::existsAndIsCurrentThread())
    {
        // Headless test mode → disable async callbacks directly
        apvts.state = {};
    }

    DBG("releaseResources end");
}

bool MIDIControl001AudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() != juce::AudioChannelSet::disabled();
}

// --- NEW: build a ParameterSnapshot each block from APVTS ---
ParameterSnapshot MIDIControl001AudioProcessor::makeSnapshotFromParams() const
{
    ParameterSnapshot s;
    // IDs assumed to exist in ParameterIDs.h / ParamLayout:
    // masterVolume, masterMix, oscFreq, envAttack, envRelease

    if (auto* p = apvts.getRawParameterValue(ParameterIDs::masterVolume)) s.masterVolumeDb = p->load();
    if (auto* p = apvts.getRawParameterValue(ParameterIDs::masterMix))    s.masterMix      = p->load();
    if (auto* p = apvts.getRawParameterValue(ParameterIDs::oscFreq))      s.oscFreq        = p->load();
    if (auto* p = apvts.getRawParameterValue(ParameterIDs::envAttack))    s.envAttack      = p->load();
    if (auto* p = apvts.getRawParameterValue(ParameterIDs::envRelease))   s.envRelease     = p->load();

    return s;
}

void MIDIControl001AudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                                juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear(); // ✅ clear incoming host buffer once

    const int numSamples   = buffer.getNumSamples();
    const int numChannels  = buffer.getNumChannels();

    // ensure scratch matches current block size
    if (monoScratch_.getNumSamples() != numSamples)
        monoScratch_.setSize(1, numSamples);
    monoScratch_.clear();

    // 1) snapshot parameters
    const auto snap = makeSnapshotFromParams();
    voiceManager_.startBlock(snap);

    // 2) feed MIDI
    for (const auto metadata : midi)
    {
        const auto msg = metadata.getMessage();
        if      (msg.isNoteOn())  voiceManager_.handleNoteOn (msg.getNoteNumber(), msg.getFloatVelocity());
        else if (msg.isNoteOff()) voiceManager_.handleNoteOff(msg.getNoteNumber());
    }

    // 3) render mono into scratch
    float* mono = monoScratch_.getWritePointer(0);
    voiceManager_.render(mono, numSamples);

    // ❌ REMOVE this line
    // buffer.clear();

    // 4) mix to output
    const float mix  = juce::jlimit(0.0f, 1.0f, snap.masterMix);
    const float gain = juce::Decibels::decibelsToGain(snap.masterVolumeDb);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* out = buffer.getWritePointer(ch);
        for (int i = 0; i < numSamples; ++i)
            out[i] = mono[i] * mix * gain;
    }
}

void MIDIControl001AudioProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, dest);
}

void MIDIControl001AudioProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
    {
        auto vt = juce::ValueTree::fromXml(*xml);
        if (vt.isValid())
            apvts.replaceState(vt);
    }
}

juce::AudioProcessorEditor* MIDIControl001AudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MIDIControl001AudioProcessor();
}
