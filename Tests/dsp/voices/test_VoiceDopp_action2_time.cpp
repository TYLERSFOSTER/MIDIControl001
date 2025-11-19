#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
using Catch::Approx;

#include "dsp/voices/VoiceDopp.h"
#include "params/ParameterSnapshot.h"

// =====================================================================
// Phase IV — Action 2 Test Suite
// Time accumulator correctness
// ---------------------------------------------------------------------
// These tests assume the existence of:
//      void enableTimeAccumulation(bool)
//
// The accumulator must:
//   ✓ Increase timeSec_ by numSamples / sampleRate
//   ✓ Have <1e–6 drift after long runs
//   ✓ Reset to 0.0 on noteOn()
//   ✓ Stay inert when disabled
// =====================================================================

TEST_CASE("VoiceDopp Action2: 1 second accumulation", "[voice][dopp][action2]")
{
    VoiceDopp v;
    v.prepare(48000.0);

    ParameterSnapshot snap;
    v.noteOn(snap, 60, 1.0f);

    v.enableTimeAccumulation(true);

    const int N = 48000;
    std::vector<float> buffer(N, 0.0f);

    v.render(buffer.data(), N);

    REQUIRE(v.getListenerTimeSeconds() == Approx(1.0).margin(1e-7));
}

TEST_CASE("VoiceDopp Action2: no drift after long run", "[voice][dopp][action2]")
{
    VoiceDopp v;
    v.prepare(48000.0);

    ParameterSnapshot snap;
    v.noteOn(snap, 60, 1.0f);

    v.enableTimeAccumulation(true);

    const int N = 48000;  // 1 second blocks
    const int K = 100;    // total 100 seconds

    std::vector<float> buffer(N, 0.0f);

    for (int i = 0; i < K; ++i)
        v.render(buffer.data(), N);

    REQUIRE(v.getListenerTimeSeconds() == Approx(100.0).margin(1e-6));
}

TEST_CASE("VoiceDopp Action2: noteOn resets time", "[voice][dopp][action2]")
{
    VoiceDopp v;
    v.prepare(48000.0);

    ParameterSnapshot snap;

    v.noteOn(snap, 60, 1.0f);
    v.enableTimeAccumulation(true);

    std::vector<float> buffer(48000, 0.0f);
    v.render(buffer.data(), 48000);

    REQUIRE(v.getListenerTimeSeconds() == Approx(1.0).margin(1e-7));

    // Reset voice
    v.noteOn(snap, 62, 1.0f);

    REQUIRE(v.getListenerTimeSeconds() == Approx(0.0).margin(1e-12));
}

TEST_CASE("VoiceDopp Action2: accumulator disabled = inert", "[voice][dopp][action2]")
{
    VoiceDopp v;
    v.prepare(48000.0);

    ParameterSnapshot snap;
    v.noteOn(snap, 60, 1.0f);

    // Do not enable accumulation
    std::vector<float> buffer(48000, 0.0f);
    v.render(buffer.data(), 48000);

    REQUIRE(v.getListenerTimeSeconds() == Approx(0.0).margin(1e-12));
}
