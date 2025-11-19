#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

using Catch::Approx;

#include "dsp/voices/VoiceDopp.h"

// Small helper for pi (we don't depend on JUCE here)
static constexpr double kPi = 3.14159265358979323846;

// ============================================================
// Action 3 — Heading and Speed Mapping
//
// θ = 2π * headingNorm − π
// v = v_max * speedNorm   (v_max = 1.0 in current implementation)
// u(θ) = (cos θ, sin θ)
// ============================================================

TEST_CASE("VoiceDopp Action3: headingNorm=0.5 gives angle 0", "[VoiceDopp][Action3]")
{
    VoiceDopp v;
    v.prepare(48000.0);

    // speed doesn't matter for heading mapping; keep it 0
    v.setListenerControls(0.0f, 0.5f); // headingNorm = 0.5

    double theta = v.computeHeadingAngle();
    REQUIRE(theta == Approx(0.0).margin(1e-12));
}

TEST_CASE("VoiceDopp Action3: headingNorm=0 gives angle -pi", "[VoiceDopp][Action3]")
{
    VoiceDopp v;
    v.prepare(48000.0);

    v.setListenerControls(0.0f, 0.0f); // headingNorm = 0

    double theta = v.computeHeadingAngle();
    REQUIRE(theta == Approx(-kPi).margin(1e-12));
}

TEST_CASE("VoiceDopp Action3: headingNorm=1 gives angle +pi", "[VoiceDopp][Action3]")
{
    VoiceDopp v;
    v.prepare(48000.0);

    v.setListenerControls(0.0f, 1.0f); // headingNorm = 1

    double theta = v.computeHeadingAngle();
    REQUIRE(theta == Approx(kPi).margin(1e-12));
}

TEST_CASE("VoiceDopp Action3: speedNorm maps linearly to speed", "[VoiceDopp][Action3]")
{
    VoiceDopp v;
    v.prepare(48000.0);

    // speedNorm = 0 → v = 0
    v.setListenerControls(0.0f, 0.5f);
    REQUIRE(v.computeSpeed() == Approx(0.0).margin(1e-12));

    // speedNorm = 1 → v = v_max = 1.0
    v.setListenerControls(1.0f, 0.5f);
    REQUIRE(v.computeSpeed() == Approx(1.0).margin(1e-12));

    // mid-point sanity check: speedNorm = 0.25 → v = 0.25
    v.setListenerControls(0.25f, 0.5f);
    REQUIRE(v.computeSpeed() == Approx(0.25).margin(1e-12));
}

TEST_CASE("VoiceDopp Action3: unit vector is consistent with heading", "[VoiceDopp][Action3]")
{
    VoiceDopp v;
    v.prepare(48000.0);

    // headingNorm = 0.5 → θ = 0 → u = (1, 0)
    v.setListenerControls(0.0f, 0.5f);
    auto u0 = v.computeUnitVector();
    REQUIRE(u0.x == Approx(1.0).margin(1e-12));
    REQUIRE(u0.y == Approx(0.0).margin(1e-12));

    // headingNorm = 0 → θ = -π → u = (-1, 0)
    v.setListenerControls(0.0f, 0.0f);
    auto uNegX = v.computeUnitVector();
    REQUIRE(uNegX.x == Approx(-1.0).margin(1e-12));
    REQUIRE(uNegX.y == Approx(0.0).margin(1e-12));

    // headingNorm = 0.25 → θ = -π/2 → u ≈ (0, -1)
    v.setListenerControls(0.0f, 0.25f);
    auto uNegY = v.computeUnitVector();
    REQUIRE(uNegY.x == Approx(0.0).margin(1e-12));
    REQUIRE(uNegY.y == Approx(-1.0).margin(1e-12));
}
