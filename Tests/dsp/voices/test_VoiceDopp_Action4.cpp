#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "dsp/voices/VoiceDopp.h"

using Catch::Approx;

// ============================================================
// Action 4 — Listener Trajectory Integration
//
// When enableTimeAccumulation(true):
//   dt = numSamples / sampleRate
//   θ = computeHeadingAngle()
//   u = (cos θ, sin θ)
//   v = computeSpeed()
//   listenerPos += (v * u * dt)
//
// Tests verify numerical correctness only.
// ============================================================

TEST_CASE("VoiceDopp Action4: position remains fixed when speed=0", "[VoiceDopp][Action4]")
{
    VoiceDopp v;
    v.prepare(48000.0);

    v.setListenerControls(/*speed*/0.0f, /*heading*/0.25f);
    v.enableTimeAccumulation(true);

    float buffer[128];
    v.noteOn(ParameterSnapshot{}, 60, 1.0f);
    v.render(buffer, 128);  // any dt

    auto pos = v.getListenerPosition();
    REQUIRE(pos.x == Approx(0.0f).margin(1e-9f));
    REQUIRE(pos.y == Approx(0.0f).margin(1e-9f));
}

TEST_CASE("VoiceDopp Action4: heading=0.5 (θ=0) moves +X only", "[VoiceDopp][Action4]")
{
    VoiceDopp v;
    v.prepare(48000.0);

    v.setListenerControls(/*speed*/1.0f, /*heading*/0.5f); // θ = 0 → unit = (1, 0)
    v.enableTimeAccumulation(true);

    constexpr int N = 480; // dt = 480/48000 = 0.01s
    float buffer[N];

    v.noteOn(ParameterSnapshot{}, 60, 1.0f);
    v.render(buffer, N);

    auto pos = v.getListenerPosition();
    REQUIRE(pos.x == Approx(0.01f).margin(1e-9f));
    REQUIRE(pos.y == Approx(0.0f).margin(1e-9f));
}

TEST_CASE("VoiceDopp Action4: heading=0 (θ=-π) moves -X only", "[VoiceDopp][Action4]")
{
    VoiceDopp v;
    v.prepare(48000.0);

    v.setListenerControls(/*speed*/1.0f, /*heading*/0.0f); // θ=-π → unit=(-1,0)
    v.enableTimeAccumulation(true);

    constexpr int N = 480; // dt = 0.01
    float buffer[N];

    v.noteOn(ParameterSnapshot{}, 60, 1.0f);
    v.render(buffer, N);

    auto pos = v.getListenerPosition();
    REQUIRE(pos.x == Approx(-0.01f).margin(1e-9f));
    REQUIRE(pos.y == Approx(0.0f).margin(1e-9f));
}

TEST_CASE("VoiceDopp Action4: heading=0.25 (θ=-π/2) moves -Y only", "[VoiceDopp][Action4]")
{
    VoiceDopp v;
    v.prepare(48000.0);

    v.setListenerControls(/*speed*/1.0f, /*heading*/0.25f); // θ = -π/2 → (0, -1)
    v.enableTimeAccumulation(true);

    constexpr int N = 480; // dt = 0.01
    float buffer[N];

    v.noteOn(ParameterSnapshot{}, 60, 1.0f);
    v.render(buffer, N);

    auto pos = v.getListenerPosition();
    REQUIRE(pos.x == Approx(0.0f).margin(1e-9f));
    REQUIRE(pos.y == Approx(-0.01f).margin(1e-9f));
}

TEST_CASE("VoiceDopp Action4: cumulative integration over multiple blocks", "[VoiceDopp][Action4]")
{
    VoiceDopp v;
    v.prepare(48000.0);

    // Move at speed 0.5 along +X (θ=0)
    v.setListenerControls(/*speed*/0.5f, /*heading*/0.5f);
    v.enableTimeAccumulation(true);

    constexpr int N = 480; // each render → dt = 0.01s
    float buffer[N];

    v.noteOn(ParameterSnapshot{}, 60, 1.0f);

    // 3 blocks → total dt = 0.03s → distance = v * t = 0.5 * 0.03 = 0.015
    v.render(buffer, N);
    v.render(buffer, N);
    v.render(buffer, N);

    auto pos = v.getListenerPosition();
    REQUIRE(pos.x == Approx(0.015f).margin(1e-9f));
    REQUIRE(pos.y == Approx(0.0f).margin(1e-9f));
}
