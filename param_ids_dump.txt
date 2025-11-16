#pragma once
namespace ParameterIDs {
    // ============================================================
    // Master-level parameters
    // ============================================================
    inline constexpr auto masterVolume    = "master/volume";
    inline constexpr auto masterMix       = "master/mix";

    // ============================================================
    // Global voice-mode selector (Phase II)
    // ============================================================
    inline constexpr auto voiceMode       = "voice/mode";

    // ============================================================
    // Per-voice DSP parameters
    // ============================================================
    inline constexpr auto oscType         = "osc/type";
    inline constexpr auto oscFreq         = "osc/freq";
    inline constexpr auto envAttack       = "env/attack";
    inline constexpr auto envRelease      = "env/release";

    // ============================================================
    // Scope parameters (GUI only)
    // ============================================================
    inline constexpr auto scopeEnabled    = "scope/enable";
    inline constexpr auto scopeBrightness = "scope/brightness";
}

// ============================================================
// Voice configuration constants
// ============================================================
constexpr int NUM_VOICES = 3;
