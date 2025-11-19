#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "dsp/voices/VoiceDopp.h"
#include "params/ParameterSnapshot.h"

#include <cmath>

TEST_CASE("VoiceDopp Action6: distance computation matches Euclidean norm", "[VoiceDopp][Action6]")
{
    VoiceDopp v;
    v.prepare(48000.0);

    // Listener at origin by default
    REQUIRE(v.getListenerPosition().x == Catch::Approx(0.0f));
    REQUIRE(v.getListenerPosition().y == Catch::Approx(0.0f));

    // Simple emitter positions
    juce::Point<float> e1{3.0f, 4.0f};  // distance = 5
    juce::Point<float> e2{-2.0f, 2.0f}; // distance = sqrt(8)

    double d1 = v.computeDistanceToEmitter(e1);
    double d2 = v.computeDistanceToEmitter(e2);

    REQUIRE(d1 == Catch::Approx(5.0).margin(1e-12));
    REQUIRE(d2 == Catch::Approx(std::sqrt(8.0)).margin(1e-12));
}

TEST_CASE("VoiceDopp Action6: retarded time always <= listener time", "[VoiceDopp][Action6]")
{
    VoiceDopp v;
    v.prepare(48000.0);

    // Activate the voice so render() actually runs the kinematics
    ParameterSnapshot snapshot;
    v.noteOn(snapshot, 60, 1.0f);

    // Enable time accumulation and advance by 1 second
    v.enableTimeAccumulation(true);
    float buffer[48000] = {};
    v.render(buffer, 48000);   // +1 second

    REQUIRE(v.getListenerTimeSeconds() == Catch::Approx(1.0).margin(1e-9));

    // Put an emitter somewhere
    juce::Point<float> e{10.0f, 0.0f};
    double dist = v.computeDistanceToEmitter(e);

    double tRet = v.computeRetardedTime(dist);

    // Must satisfy t_ret = t - r/c <= t
    REQUIRE(tRet <= v.getListenerTimeSeconds());

    // Must be strictly less for any nonzero distance
    REQUIRE(tRet < v.getListenerTimeSeconds());
}

TEST_CASE("VoiceDopp Action6: approaching emitter increases retarded time", "[VoiceDopp][Action6]")
{
    VoiceDopp v;
    v.prepare(48000.0);

    // Activate the voice
    ParameterSnapshot snapshot;
    v.noteOn(snapshot, 60, 1.0f);

    // Enable motion
    v.enableTimeAccumulation(true);

    // Move toward emitter along +X
    v.setListenerControls(1.0f, 0.5f);  // speed = vMax, heading = 0 rad

    // Emitter at +10m
    juce::Point<float> e{10.0f, 0.0f};

    float block[48000] = {};

    // --- t0 ---
    v.render(block, 48000); // t ≈ 1s
    double t0  = v.getListenerTimeSeconds();
    double d0  = v.computeDistanceToEmitter(e);
    double tr0 = v.computeRetardedTime(d0);

    // --- t1 ---
    v.render(block, 48000); // t ≈ 2s
    double t1  = v.getListenerTimeSeconds();
    double d1  = v.computeDistanceToEmitter(e);
    double tr1 = v.computeRetardedTime(d1);

    REQUIRE(t1 > t0);

    // Listener moved closer → distance decreased → retardation term smaller → t_ret increases
    REQUIRE(d1 < d0);
    REQUIRE(tr1 > tr0);
}

TEST_CASE("VoiceDopp Action6: receding emitter retarded time grows more slowly than listener time", "[VoiceDopp][Action6]")
{
    VoiceDopp v;
    v.prepare(48000.0);

    // Activate the voice
    ParameterSnapshot snapshot;
    v.noteOn(snapshot, 60, 1.0f);

    v.enableTimeAccumulation(true);

    // Move away from emitter along -X:
    // headingNorm = 1 → θ = π → unit vector (-1, 0)
    v.setListenerControls(1.0f, 1.0f);

    // Emitter at +10m
    juce::Point<float> e{10.0f, 0.0f};

    float block[48000] = {};

    // --- t0 ---
    v.render(block, 48000);
    double t0  = v.getListenerTimeSeconds();
    double d0  = v.computeDistanceToEmitter(e);
    double tr0 = v.computeRetardedTime(d0);

    // --- t1 ---
    v.render(block, 48000);
    double t1  = v.getListenerTimeSeconds();
    double d1  = v.computeDistanceToEmitter(e);
    double tr1 = v.computeRetardedTime(d1);

    // Distance increased (moving away)
    REQUIRE(t1 > t0);
    REQUIRE(d1 > d0);

    // Retarded time still increases (dt_ret/dt > 0 when |v| << c),
    // but it grows more slowly than listener time.
    REQUIRE(tr1 > tr0);
    REQUIRE((tr1 - tr0) < (t1 - t0));
}

TEST_CASE("VoiceDopp Action6: retarded time well-defined even for large distances", "[VoiceDopp][Action6]")
{
    VoiceDopp v;
    v.prepare(48000.0);

    // Activate the voice so time advances
    ParameterSnapshot snapshot;
    v.noteOn(snapshot, 60, 1.0f);

    v.enableTimeAccumulation(true);

    float block[48000] = {};
    v.render(block, 48000); // t ≈ 1s

    // Very large coordinate
    juce::Point<float> e{10000.0f, 0.0f}; // 10 km away
    double d  = v.computeDistanceToEmitter(e);
    double tr = v.computeRetardedTime(d);

    // Still t_ret = t - r/c
    REQUIRE(tr == Catch::Approx(1.0 - d / 343.0).margin(1e-9));
}
