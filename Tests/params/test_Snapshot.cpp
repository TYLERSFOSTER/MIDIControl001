#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
using Catch::Approx;
#include "params/ParameterSnapshot.h"

TEST_CASE("ParameterSnapshot default values", "[snapshot]") {
    ParameterSnapshot snap;

    REQUIRE(snap.masterVolumeDb == Approx(-6.0f));
    REQUIRE(snap.masterMix      == Approx(1.0f));
    REQUIRE(snap.oscFreq        == Approx(440.0f));
    REQUIRE(snap.envAttack      == Approx(0.01f));
    REQUIRE(snap.envRelease     == Approx(0.2f));
}

TEST_CASE("ParameterSnapshot assignment and copy", "[snapshot]") {
    ParameterSnapshot a;
    a.masterVolumeDb = 0.0f;
    a.oscFreq = 880.0f;

    ParameterSnapshot b = a;  // copy

    REQUIRE(b.masterVolumeDb == Approx(0.0f));
    REQUIRE(b.oscFreq == Approx(880.0f));

    // Modify 'a' should not affect 'b'
    a.oscFreq = 220.0f;
    REQUIRE(b.oscFreq == Approx(880.0f));
}
