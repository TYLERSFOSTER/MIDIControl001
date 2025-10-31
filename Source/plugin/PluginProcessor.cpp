#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "params/ParameterIDs.h"

static inline float ccTo01(int value)
{
    return juce::jlimit(0.0f, 1.0f, value / 127.0f);
}

// ============================================================
// Parameter layout definition (diagnostic version)
// ============================================================

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    DBG("=== Building ParameterLayout ===");

    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    DBG("Adding parameter: masterVolume");
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParameterIDs::masterVolume,
        "Master Volume",
        juce::NormalisableRange<float>(-60.0f, 0.0f),
        -6.0f));

    DBG("Adding parameter: masterMix");
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParameterIDs::masterMix,
        "Master Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        1.0f));

    DBG("Adding parameter: oscFreq");
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParameterIDs::oscFreq,
        "Osc Frequency",
        juce::NormalisableRange<float>(20.0f, 20000.0f, 0.01f, 0.3f),
        440.0f));

    DBG("Adding parameter: envAttack");
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParameterIDs::envAttack,
        "Env Attack",
        juce::NormalisableRange<float>(0.001f, 2.0f),
        0.01f));

    DBG("Adding parameter: envRelease");
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParameterIDs::envRelease,
        "Env Release",
        juce::NormalisableRange<float>(0.01f, 5.0f),
        0.2f));

    DBG("=== Done Building ParameterLayout ===");
    return layout;
}

// ============================================================
// Processor class implementation
// ============================================================

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
        apvts.state = {};
    }

    DBG("releaseResources end");
}

bool MIDIControl001AudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() != juce::AudioChannelSet::disabled();
}

// ============================================================
// Snapshot diagnostics
// ============================================================

ParameterSnapshot MIDIControl001AudioProcessor::makeSnapshotFromParams() const
{
    ParameterSnapshot s;

    if (auto* p = apvts.getRawParameterValue(ParameterIDs::masterVolume)) s.masterVolumeDb = p->load();
    if (auto* p = apvts.getRawParameterValue(ParameterIDs::masterMix))    s.masterMix      = p->load();
    if (auto* p = apvts.getRawParameterValue(ParameterIDs::oscFreq))      s.oscFreq        = p->load();
    if (auto* p = apvts.getRawParameterValue(ParameterIDs::envAttack))    s.envAttack      = p->load();
    if (auto* p = apvts.getRawParameterValue(ParameterIDs::envRelease))   s.envRelease     = p->load();

    DBG("Snapshot built: vol=" << s.masterVolumeDb
        << " mix=" << s.masterMix
        << " atk=" << s.envAttack
        << " rel=" << s.envRelease);

    return s;
}

// ============================================================
// Audio/MIDI processing
// ============================================================

void MIDIControl001AudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                                juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    const int numSamples   = buffer.getNumSamples();
    const int numChannels  = buffer.getNumChannels();

    if (monoScratch_.getNumSamples() != numSamples)
        monoScratch_.setSize(1, numSamples);
    monoScratch_.clear();

    const auto snap = makeSnapshotFromParams();
    voiceManager_.startBlock(snap);

    for (const auto metadata : midi)
    {
        const auto msg = metadata.getMessage();

        DBG("MIDI message: " << msg.getDescription());

        if (msg.isNoteOn())
            DBG("  NoteOn  #" << msg.getNoteNumber()
                << " → freq=" << 440.0f * std::pow(2.0f, (msg.getNoteNumber() - 69) / 12.0f));

        if (msg.isController())
            DBG("  Controller #" << msg.getControllerNumber()
                << " value=" << msg.getControllerValue());

        if (msg.isController())
        {
            const int cc = msg.getControllerNumber();
            const int val = msg.getControllerValue();
            const float norm = ccTo01(val);

            switch (cc)
            {
                case 1:
                    if (auto* p = apvts.getParameter(ParameterIDs::masterVolume))
                        p->setValueNotifyingHost(norm);
                    DBG("Mapped CC#1 (Mod Wheel) to masterVolume = " << norm);
                    break;

                case 2:
                    if (auto* p = apvts.getParameter(ParameterIDs::masterMix))
                        p->setValueNotifyingHost(norm);
                    DBG("Mapped CC#2 (Breath) to masterMix = " << norm);
                    break;

                default:
                    break;
            }
        }

        if      (msg.isNoteOn())  voiceManager_.handleNoteOn (msg.getNoteNumber(), msg.getFloatVelocity());
        else if (msg.isNoteOff()) voiceManager_.handleNoteOff(msg.getNoteNumber());
    }

    float* mono = monoScratch_.getWritePointer(0);
    voiceManager_.render(mono, numSamples);

    const float mix  = juce::jlimit(0.0f, 1.0f, snap.masterMix);
    const float gain = juce::Decibels::decibelsToGain(snap.masterVolumeDb);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* out = buffer.getWritePointer(ch);
        for (int i = 0; i < numSamples; ++i)
            out[i] = mono[i] * mix * gain;
    }
}

// ============================================================
// State / Editor
// ============================================================

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
    static int editorCount = 0;
    ++editorCount;
    DBG("=== createEditor() called! Instance #" << editorCount << " ===");

    return new juce::GenericAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MIDIControl001AudioProcessor();
}
