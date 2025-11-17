#include <catch2/catch_test_macros.hpp>
#include "dsp/VoiceManager.h"
#include "params/ParameterSnapshot.h"

// Dummy snapshot generator
static ParameterSnapshot makeSnap()
{
    ParameterSnapshot s;
    s.voiceMode = VoiceMode::VoiceA; // default
    return s;
}

TEST_CASE("VoiceManager: mode wiring and factory selection", "[voice][manager][C1]")
{
    VoiceManager vm(makeSnap);

    // Prepare with a normal sample rate
    vm.prepare(48000.0);

    // ===== Test: default mode =====
    REQUIRE(vm.getMode() == VoiceMode::VoiceA);

    // ===== Test: setMode updates internal state =====
    vm.setMode(VoiceMode::VoiceDopp);
    REQUIRE(vm.getMode() == VoiceMode::VoiceDopp);

    // Ensure the rebuild logic sees a mode change
    vm.startBlock(); // triggers B8 rebuild-if-changed logic

    // ===== Test: voice factory dispatch =====
    // We intentionally check *type identity*, not behavior.
    auto v = vm.makeVoiceForMode(VoiceMode::VoiceDopp);
    REQUIRE(dynamic_cast<VoiceDopp*>(v.get()) != nullptr);

    auto w = vm.makeVoiceForMode(VoiceMode::VoiceA);
    REQUIRE(dynamic_cast<VoiceA*>(w.get()) != nullptr);

    // ===== Test: injectable factory =====
    bool fmUsed = false;

    vm.setVoiceFactory([&](VoiceMode){
        fmUsed = true;
        return std::make_unique<VoiceA>(); // arbitrary
    });

    vm.prepare(48000.0); // forces rebuild

    REQUIRE(fmUsed == true);
}
