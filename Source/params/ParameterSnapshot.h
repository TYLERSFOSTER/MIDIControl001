#pragma once
#include <array>
#include <cstddef>
#include "ParameterIDs.h"

// ============================================================
// Voice mode enumeration (Phase III scaffolding)
// ------------------------------------------------------------
// IMPORTANT: Order must match ParamLayout.cpp:
//
//   0 -> VoiceA
//   1 -> VoiceDopp
//   2 -> VoiceLET
//   3 -> VoiceFM
//
// For now, the engine still instantiates only VoiceA for *all*
// modes. That keeps DSP output identical while allowing the
// plugin to carry a typed mode flag for future expansion.
// ============================================================
enum class VoiceMode : int
{
    VoiceA    = 0,
    VoiceDopp = 1,
    VoiceLET  = 2,
    VoiceFM   = 3,
};

inline VoiceMode toVoiceMode(int raw)
{
    switch (raw)
    {
        case 0:  return VoiceMode::VoiceA;
        case 1:  return VoiceMode::VoiceDopp;
        case 2:  return VoiceMode::VoiceLET;
        case 3:  return VoiceMode::VoiceFM;
        default: return VoiceMode::VoiceA;  // clamp bad indices
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
