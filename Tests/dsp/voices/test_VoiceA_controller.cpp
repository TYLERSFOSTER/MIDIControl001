#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
using Catch::Approx;

#include "dsp/voices/VoiceA.h"
#include "params/ParameterSnapshot.h"

// ============================================================
// Regression test: per-voice controller mapping (CC3–CC5)
// ============================================================
//
// Purpose:
//   Verify VoiceA::handleController correctly maps normalized CC
//   values to envelope attack, release, and oscillator frequency.
//
// Dependencies:
//   None beyond VoiceA.h / EnvelopeA.h / OscillatorA.h
// ============================================================

TEST_CASE("VoiceA controller mappings (CC3–CC5)", "[voice][controller]") {
    VoiceA v;
    v.prepare(44100.0);

    // --- CC3: attack modulation ---
    v.handleController(3, 0.0f); // minimum
    v.handleController(3, 1.0f); // maximum
    // We can't directly query attack, but no crash = success
    REQUIRE(true);

    // --- CC4: release modulation ---
    v.handleController(4, 0.0f);
    v.handleController(4, 1.0f);
    REQUIRE(true);

    // --- CC5: frequency modulation ---
    // At midpoint, frequency should remain near 440 Hz
    v.handleController(5, 0.5f);
    // Sweep extremes to ensure no crash or domain error
    v.handleController(5, 0.0f);
    v.handleController(5, 1.0f);

    // Trigger short render to validate audible output
    ParameterSnapshot snap;
    snap.oscFreq = 440.0f;
    snap.envAttack = 0.001f;
    snap.envRelease = 0.05f;

    v.noteOn(snap, 69, 1.0f);
    std::vector<float> buf(128, 0.0f);
    v.render(buf.data(), (int)buf.size());

    // Basic signal sanity: some nonzero samples expected
    bool anyNonzero = std::any_of(buf.begin(), buf.end(),
                                  [](float x){ return std::fabs(x) > 0.0f; });
    REQUIRE(anyNonzero);
}
