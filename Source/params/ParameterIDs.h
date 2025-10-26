#pragma once
namespace ParameterIDs
{
    // Master
    inline constexpr auto masterVolume   = "master/volume";
    inline constexpr auto masterMix      = "master/mix";

    // Voice shared
    inline constexpr auto oscType        = "osc/type";
    inline constexpr auto oscFreq        = "osc/freq";
    inline constexpr auto envAttack      = "env/attack";
    inline constexpr auto envRelease     = "env/release";
    inline constexpr auto scopeEnabled   = "scope/enable";
    inline constexpr auto scopeBrightness= "scope/brightness";
}
