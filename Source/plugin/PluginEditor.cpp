#include "PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>

juce::AudioProcessorEditor* MIDIControl001AudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor(*this);
}
