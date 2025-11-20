#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
using Catch::Approx;

#include "dsp/voices/VoiceDopp.h"

// ============================================================
// Action-9 tests: predictive lattice window + best emitter
// ============================================================

TEST_CASE("VoiceDopp Action9: best emitter along +X heading")
{
    VoiceDopp v;
    v.prepare(48000.0);

    // Keep the listener time accumulator off: pure kinematic snapshot.
    v.enableTimeAccumulation(false);

    // Move along +X: headingNorm = 0.5 => θ = 0.
    v.setListenerControls(/*speedNorm=*/1.0f, /*headingNorm=*/0.5f);

    // Axis-aligned lattice: orientationNorm = 0.5 => φ = 0, so
    // n = (1,0), b = (0,1), and x_{k,m} = (k, m).
    v.setEmitterFieldControls(/*densityNorm=*/1.0f,
                              /*orientationNorm=*/0.5f);

    // Consider emitters at (-1,0), (0,0), (1,0).
    VoiceDopp::EmitterCandidate best =
        v.findBestEmitterInWindow(/*kMin=*/-1, /*kMax=*/1,
                                  /*mMin=*/ 0, /*mMax=*/0);

    // We expect the emitter ahead of the listener along +X (k=+1)
    // to have the best predictive score.
    REQUIRE(best.k == 1);
    REQUIRE(best.m == 0);
}

TEST_CASE("VoiceDopp Action9: symmetric Y emitters, on-axis emitter dominates")
{
    VoiceDopp v;
    v.prepare(48000.0);
    v.enableTimeAccumulation(false);

    // Listener moving along +X (θ = 0).
    v.setListenerControls(/*speedNorm=*/1.0f, /*headingNorm=*/0.5f);

    // Axis-aligned lattice: x_{k,m} = (k, m).
    v.setEmitterFieldControls(/*densityNorm=*/1.0f,
                              /*orientationNorm=*/0.5f);

    // ------------------------------------------------------------
    // Diagnostics: orientation and velocity MUST match the spec.
    // ------------------------------------------------------------
    {
        auto n = v.computeEmitterNormal();
        auto b = v.computeEmitterTangent();

        INFO("EmitterNormal n = (" << n.x << ", " << n.y << ")");
        INFO("EmitterTangent b = (" << b.x << ", " << b.y << ")");

        // These MUST be exact (up to tiny FP error) for symmetry:
        // n = (1, 0)
        // b = (0, 1)
        REQUIRE(n.x == Approx(1.0f).margin(1e-6f));
        REQUIRE(n.y == Approx(0.0f).margin(1e-6f));
        REQUIRE(b.x == Approx(0.0f).margin(1e-6f));
        REQUIRE(b.y == Approx(1.0f).margin(1e-6f));
    }
    {
        auto vel = v.getListenerVelocity();
        INFO("listenerVel = (" << vel.x << ", " << vel.y << ")");

        // v = (1, 0) in our normalized units.
        REQUIRE(vel.x == Approx(1.0f).margin(1e-6f));
        REQUIRE(vel.y == Approx(0.0f).margin(1e-6f));
    }

    // ------------------------------------------------------------
    // Check scores for the three emitters at k=1, m=-1,0,+1
    // ------------------------------------------------------------
    auto posMinus = v.computeEmitterPosition(/*k=*/1, /*m=*/-1);
    auto posCenter = v.computeEmitterPosition(/*k=*/1, /*m=*/ 0);
    auto posPlus  = v.computeEmitterPosition(/*k=*/1, /*m=*/ 1);

    double sMinus  = v.computePredictiveScoreForEmitter(posMinus);
    double sCenter = v.computePredictiveScoreForEmitter(posCenter);
    double sPlus   = v.computePredictiveScoreForEmitter(posPlus);

    INFO("scores: m=-1 -> " << sMinus
         << ", m=0 -> " << sCenter
         << ", m=+1 -> " << sPlus);

    // Symmetry in Y: scores for m = -1 and m = +1 must match.
    REQUIRE(sMinus == Approx(sPlus).margin(1e-9));

    // The on-axis emitter (m = 0) must dominate.
    REQUIRE(sCenter > sMinus);

    // ------------------------------------------------------------
    // Now let the lattice-window selector pick the best.
    // ------------------------------------------------------------
    VoiceDopp::EmitterCandidate best =
        v.findBestEmitterInWindow(/*kMin=*/1, /*kMax=*/1,
                                  /*mMin=*/-1, /*mMax=*/1);

    INFO("best.k = " << best.k
         << ", best.m = " << best.m
         << ", best.score = " << best.score);

    REQUIRE(best.k == 1);
    REQUIRE(best.m == 0);
}
