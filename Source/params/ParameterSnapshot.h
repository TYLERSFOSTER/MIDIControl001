#pragma once
#include <array>
#include <cstddef>
#include "ParameterIDs.h"

// ============================================================
// Voice parameter bundle (per-voice settings)
// ============================================================
struct VoiceParams
{
    float oscFreq   = 440.0f;
    float envAttack = 0.01f;
    float envRelease = 0.2f;
};

// Simple immutable snapshot of all relevant parameter values.
// Built once per audio block by the PluginProcessor and passed
// to the VoiceManager so all voices share a consistent block state.

struct ParameterSnapshot {
    float masterVolumeDb = -6.0f;
    float masterMix      = 1.0f;
    float oscFreq        = 440.0f;
    float envAttack      = 0.01f;
    float envRelease     = 0.2f;

    // Per-voice parameter data
    std::array<VoiceParams, NUM_VOICES> voices {};
};
