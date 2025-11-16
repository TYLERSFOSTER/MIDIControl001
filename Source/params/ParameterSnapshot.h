#pragma once
#include <array>
#include <cstddef>
#include "ParameterIDs.h"

// ============================================================
// Voice mode enumeration (Phase III scaffolding)
// ============================================================
enum class VoiceMode : int
{
    VoiceA = 0,
    // Future: VoiceLET = 1,
    //         VoiceDopp = 2,
    //         ...
};

inline VoiceMode toVoiceMode(int raw)
{
    // For now only VoiceA exists; clamp everything to VoiceA.
    // This keeps behavior identical regardless of host value.
    switch (raw)
    {
        case 0:
        default:
            return VoiceMode::VoiceA;
    }
}

inline int toInt(VoiceMode m)
{
    return static_cast<int>(m);
}

// ============================================================
// Voice parameter bundle (per-voice settings)
// ============================================================
struct VoiceParams
{
    float oscFreq    = 440.0f;
    float envAttack  = 0.01f;
    float envRelease = 0.2f;
};

// Simple immutable snapshot of all relevant parameter values.
// Built once per audio block by the PluginProcessor and passed
// to the VoiceManager so all voices share a consistent block state.

struct ParameterSnapshot {
    float     masterVolumeDb = -6.0f;
    float     masterMix      = 1.0f;
    VoiceMode voiceMode      = VoiceMode::VoiceA; // 0 == "voiceA"
    float     oscFreq        = 440.0f;
    float     envAttack      = 0.01f;
    float     envRelease     = 0.2f;

    // Per-voice parameter data
    std::array<VoiceParams, NUM_VOICES> voices {};
};
