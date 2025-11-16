#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "params/ParameterIDs.h"
#include "params/ParamLayout.h"

static inline float ccTo01(int value)
{
    return juce::jlimit(0.0f, 1.0f, value / 127.0f);
}

// ============================================================
// Processor class implementation
// ============================================================

MIDIControl001AudioProcessor::MIDIControl001AudioProcessor()
  : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    apvts(*this, nullptr, "Parameters", createParameterLayout()),
    voiceManager_([this]{ return makeSnapshotFromParams(); })
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

    // ============================================================
    // NEW FOR PHASE II — read global voice mode
    // ============================================================
    if (auto* p = apvts.getRawParameterValue(ParameterIDs::voiceMode))
        s.voiceMode = static_cast<int>(p->load());

    if (auto* p = apvts.getRawParameterValue(ParameterIDs::oscFreq))      s.oscFreq        = p->load();
    if (auto* p = apvts.getRawParameterValue(ParameterIDs::envAttack))    s.envAttack      = p->load();
    if (auto* p = apvts.getRawParameterValue(ParameterIDs::envRelease))   s.envRelease     = p->load();

    // ============================================================
    // Per-voice parameter group reads
    // ============================================================
    for (int i = 0; i < NUM_VOICES; ++i)
    {
        VoiceParams vp;

        const juce::String prefix = "voices/voice" + juce::String(i + 1) + "/";

        if (auto* p = apvts.getRawParameterValue(prefix + "osc/freq"))
            vp.oscFreq = p->load();

        if (auto* p = apvts.getRawParameterValue(prefix + "env/attack"))
            vp.envAttack = p->load();

        if (auto* p = apvts.getRawParameterValue(prefix + "env/release"))
            vp.envRelease = p->load();

        s.voices[i] = vp;

        DBG("Voice" << (i + 1)
            << " freq=" << vp.oscFreq
            << " atk=" << vp.envAttack
            << " rel=" << vp.envRelease);
    }

    DBG("Snapshot built: vol=" << s.masterVolumeDb
        << " mix=" << s.masterMix
        << " mode=" << s.voiceMode
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

    // Phase II A5 — forward mode into VoiceManager
    voiceManager_.setMode(snap.voiceMode);

    voiceManager_.startBlock();

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

            voiceManager_.handleController(cc, norm);
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
