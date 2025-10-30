#pragma once
#include <juce_gui_extra/juce_gui_extra.h>
#include "../plugin/PluginProcessor.h"

class SimpleVolumeGUI : public juce::AudioProcessorEditor
{
public:
    SimpleVolumeGUI(MIDIControl001AudioProcessor& p)
        : juce::AudioProcessorEditor(&p), processor(p)
    {
        addAndMakeVisible(volumeSlider);
        volumeSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        volumeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);

        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.apvts, ParameterIDs::masterVolume, volumeSlider);

        setSize(200, 200);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::black);
        g.setColour(juce::Colours::white);
        g.drawFittedText("Master Volume", getLocalBounds().removeFromTop(20),
                         juce::Justification::centred, 1);
    }

    void resized() override
    {
        volumeSlider.setBounds(getLocalBounds().reduced(40));
    }

private:
    MIDIControl001AudioProcessor& processor;
    juce::Slider volumeSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};
