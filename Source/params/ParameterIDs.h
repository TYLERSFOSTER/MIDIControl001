#pragma once
namespace ParameterIDs {
    inline constexpr auto masterVolume    = "master/volume";
    inline constexpr auto masterMix       = "master/mix";

    inline constexpr auto oscType         = "osc/type";
    inline constexpr auto oscFreq         = "osc/freq";
    inline constexpr auto envAttack       = "env/attack";
    inline constexpr auto envRelease      = "env/release";
    inline constexpr auto scopeEnabled    = "scope/enable";
    inline constexpr auto scopeBrightness = "scope/brightness";
}

// ============================================================
// Voice configuration constants
// ============================================================
constexpr int NUM_VOICES = 3;
