#pragma once

#include <JuceHeader.h>

// ============================================================
// ParamLayout.h
// ------------------------------------------------------------
// Declares the factory function that builds the parameter
// hierarchy for the AudioProcessorValueTreeState (APVTS).
// This layout defines all master and per-voice parameters
// used throughout the plugin.
//
// Implemented in: ParamLayout.cpp
// ============================================================

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
