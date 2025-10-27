#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// Constructor & destructor
MIDIControl001AudioProcessor::MIDIControl001AudioProcessor()
{
}

//==============================================================================
// The required factory function that JUCE expects
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MIDIControl001AudioProcessor();
}
