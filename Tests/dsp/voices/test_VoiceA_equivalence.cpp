// ============================================================================
// test_VoiceA_equivalence.cpp
// Ensures that VoiceA output matches VoiceLegacy sample-for-sample.
// ============================================================================

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
using Catch::Approx;

#include <cmath>
#include <vector>

#include "dsp/legacy/VoiceLegacy.h"   // old version (baseline reference)
#include "dsp/voices/VoiceA.h"        // new version under test
#include "params/ParameterSnapshot.h" // parameter struct (shared)

// ----------------------------------------------------------------------------
// Compare VoiceA and VoiceLegacy for sample-level equivalence
// ----------------------------------------------------------------------------
TEST_CASE("VoiceA matches VoiceLegacy output", "[voice][equivalence]") {
    ParameterSnapshot snap;
    snap.oscFreq    = 440.0f;
    snap.envAttack  = 0.001f;
    snap.envRelease = 0.01f;

    VoiceLegacy legacy;
    VoiceA      modern;

    legacy.prepare(44100.0);
    modern.prepare(44100.0);

    legacy.noteOn(snap, 69, 1.0f);
    modern.noteOn(snap, 69, 1.0f);

    constexpr int N = 256;
    std::vector<float> bufLegacy(N, 0.0f);
    std::vector<float> bufModern(N, 0.0f);

    legacy.render(bufLegacy.data(), N);
    modern.render(bufModern.data(), N);

    for (int i = 0; i < N; ++i)
        REQUIRE(bufModern[i] == Approx(bufLegacy[i]).margin(1e-6f));

    legacy.noteOff();
    modern.noteOff();
}
