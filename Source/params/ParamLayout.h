#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

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
