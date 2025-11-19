#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <cmath>
#include <limits>

#include "dsp/voices/VoiceDopp.h"

TEST_CASE("VoiceDopp Action5: orientation vectors match spec", "[VoiceDopp][Action5]")
{
    VoiceDopp v;
    v.prepare(48000.0);

    // orientationNorm = 0.5 -> φ = 0 -> n = (1,0), b = (0,1)
    v.setEmitterFieldControls(0.5f, 0.5f);
    auto n0 = v.computeEmitterNormal();
    auto b0 = v.computeEmitterTangent();

    REQUIRE(n0.x == Catch::Approx(1.0f).margin(1e-6f));
    REQUIRE(n0.y == Catch::Approx(0.0f).margin(1e-6f));
    REQUIRE(b0.x == Catch::Approx(0.0f).margin(1e-6f));
    REQUIRE(b0.y == Catch::Approx(1.0f).margin(1e-6f));

    // orientationNorm = 0.75 -> φ = π/2 -> n = (0,1), b = (-1,0)
    v.setEmitterFieldControls(0.5f, 0.75f);
    auto n1 = v.computeEmitterNormal();
    auto b1 = v.computeEmitterTangent();

    REQUIRE(n1.x == Catch::Approx(0.0f).margin(1e-6f));
    REQUIRE(n1.y == Catch::Approx(1.0f).margin(1e-6f));
    REQUIRE(b1.x == Catch::Approx(-1.0f).margin(1e-6f));
    REQUIRE(b1.y == Catch::Approx(0.0f).margin(1e-6f));
}

TEST_CASE("VoiceDopp Action5: deltaPerp matches density rules", "[VoiceDopp][Action5]")
{
    VoiceDopp v;
    v.prepare(48000.0);

    // ρ = 0 -> Δ⊥ = +∞ (single line k = 0 in the spec)
    v.setEmitterFieldControls(0.0f, 0.5f);
    REQUIRE(std::isinf(v.computeDeltaPerp()));

    // ρ = 0.25 -> Δ⊥ = 1 / 0.25 = 4
    v.setEmitterFieldControls(0.25f, 0.5f);
    REQUIRE(v.computeDeltaPerp() == Catch::Approx(4.0).margin(1e-12));

    // ρ = 1.0 -> Δ⊥ = 1
    v.setEmitterFieldControls(1.0f, 0.5f);
    REQUIRE(v.computeDeltaPerp() == Catch::Approx(1.0).margin(1e-12));
}

TEST_CASE("VoiceDopp Action5: emitter positions for φ = 0 (axis aligned)", "[VoiceDopp][Action5]")
{
    VoiceDopp v;
    v.prepare(48000.0);

    // φ = 0 (orientationNorm = 0.5) => n = (1,0), b = (0,1)
    // ρ = 0.5 -> Δ⊥ = 2, Δ∥ = 1
    v.setEmitterFieldControls(0.5f, 0.5f);

    auto p00 = v.computeEmitterPosition(0, 0); // (0, 0)
    auto p10 = v.computeEmitterPosition(1, 0); // (2, 0)
    auto p01 = v.computeEmitterPosition(0, 1); // (0, 1)
    auto p12 = v.computeEmitterPosition(1, 2); // (2, 2)

    REQUIRE(p00.x == Catch::Approx(0.0f).margin(1e-6f));
    REQUIRE(p00.y == Catch::Approx(0.0f).margin(1e-6f));

    REQUIRE(p10.x == Catch::Approx(2.0f).margin(1e-6f));
    REQUIRE(p10.y == Catch::Approx(0.0f).margin(1e-6f));

    REQUIRE(p01.x == Catch::Approx(0.0f).margin(1e-6f));
    REQUIRE(p01.y == Catch::Approx(1.0f).margin(1e-6f));

    REQUIRE(p12.x == Catch::Approx(2.0f).margin(1e-6f));
    REQUIRE(p12.y == Catch::Approx(2.0f).margin(1e-6f));
}

TEST_CASE("VoiceDopp Action5: emitter positions for φ = π/2 (rotated lattice)", "[VoiceDopp][Action5]")
{
    VoiceDopp v;
    v.prepare(48000.0);

    // orientationNorm = 0.75 -> φ = π/2
    // n = (0,1), b = (-1,0)
    // ρ = 0.5 -> Δ⊥ = 2, Δ∥ = 1
    v.setEmitterFieldControls(0.5f, 0.75f);

    auto p00 = v.computeEmitterPosition(0, 0); // (0, 0)
    auto p10 = v.computeEmitterPosition(1, 0); // 1 * 2 * n = (0, 2)
    auto p01 = v.computeEmitterPosition(0, 1); // 1 * b = (-1, 0)
    auto p11 = v.computeEmitterPosition(1, 1); // (0,2) + (-1,0) = (-1, 2)

    REQUIRE(p00.x == Catch::Approx(0.0f).margin(1e-6f));
    REQUIRE(p00.y == Catch::Approx(0.0f).margin(1e-6f));

    REQUIRE(p10.x == Catch::Approx(0.0f).margin(1e-6f));
    REQUIRE(p10.y == Catch::Approx(2.0f).margin(1e-6f));

    REQUIRE(p01.x == Catch::Approx(-1.0f).margin(1e-6f));
    REQUIRE(p01.y == Catch::Approx(0.0f).margin(1e-6f));

    REQUIRE(p11.x == Catch::Approx(-1.0f).margin(1e-6f));
    REQUIRE(p11.y == Catch::Approx(2.0f).margin(1e-6f));
}
