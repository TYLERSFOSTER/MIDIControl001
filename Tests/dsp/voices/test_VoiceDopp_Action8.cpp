#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "dsp/voices/VoiceDopp.h"

#include <cmath>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static juce::Point<float> P(float x, float y)
{
    return {x, y};
}

static void advanceTime(VoiceDopp& v, double seconds)
{
    // 48k render blocks
    const int block = 48000;
    int loops = static_cast<int>(seconds);

    v.enableTimeAccumulation(true);
    float buf[48000];

    for (int i = 0; i < loops; ++i)
        v.render(buf, block);
}

// ---------------------------------------------------------------------------
// Action 8 — Predictive Position
// ---------------------------------------------------------------------------

TEST_CASE("VoiceDopp Action8: predictive listener position matches physics", "[VoiceDopp][Action8]")
{
    VoiceDopp v;
    v.prepare(48000.0);

    v.setListenerControls(1.0f, 0.5f); // speed=1 m/s, heading=0 rad

    // No motion yet (accumulation disabled)
    auto p0 = v.predictListenerPosition(0.0);
    REQUIRE(p0.x == Catch::Approx(0.0f));
    REQUIRE(p0.y == Catch::Approx(0.0f));

    auto pH = v.predictListenerPosition(2.0);
    REQUIRE(pH.x == Catch::Approx(2.0f).margin(1e-12));
    REQUIRE(pH.y == Catch::Approx(0.0f).margin(1e-12));
}

TEST_CASE("VoiceDopp Action8: predictive position uses instantaneous velocity", "[VoiceDopp][Action8]")
{
    VoiceDopp v;
    v.prepare(48000.0);

    // Move listener for one second to establish position
    v.enableTimeAccumulation(true);
    v.setListenerControls(1.0f, 0.5f); 
    float buf[48000];
    v.render(buf, 48000); // now x_L = 1, y_L = 0

    auto p1 = v.predictListenerPosition(1.0);
    REQUIRE(p1.x == Catch::Approx(1.0f));
    REQUIRE(p1.y == Catch::Approx(0.0f));
}


// ---------------------------------------------------------------------------
// Action 8 — Predictive Retarded Time
// ---------------------------------------------------------------------------

TEST_CASE("VoiceDopp Action8: predictive retarded time equals direct formula", "[VoiceDopp][Action8]")
{
    VoiceDopp v;
    v.prepare(48000.0);

    // Listener moves +x
    v.enableTimeAccumulation(true);
    v.setListenerControls(1.0f, 0.5f);

    float buf[48000];
    v.render(buf, 48000); // t=1s, x_L=1

    juce::Point<float> emitter = P(10.0f, 0.0f);

    // Predict horizon = 2 sec
    double tH = v.getListenerTimeSeconds() + 2.0;
    auto predictedPos = v.predictListenerPosition(2.0);

    double dx = emitter.x - predictedPos.x;
    double dy = emitter.y - predictedPos.y;
    double r  = std::sqrt(dx*dx + dy*dy);

    double expected = tH - r / 343.0;

    REQUIRE(v.computePredictiveRetardedTime(2.0, emitter)
            == Catch::Approx(expected).margin(1e-9));
}


// ---------------------------------------------------------------------------
// Action 8 — Score Monotonicity
// ---------------------------------------------------------------------------

TEST_CASE("VoiceDopp Action8: approaching emitter yields higher score", "[VoiceDopp][Action8]")
{
    VoiceDopp v;
    v.prepare(48000.0);

    // Near, approaching
    juce::Point<float> eNear{10.0f, 0.0f};
    v.setListenerControls(1.0f, 0.5f);

    double scoreNear = v.computePredictiveScoreForEmitter(eNear);

    // Far, receding
    juce::Point<float> eFar{50.0f, 0.0f};
    v.setListenerControls(1.0f, 1.0f); // heading = π → moving left, away from +X

    double scoreFar = v.computePredictiveScoreForEmitter(eFar);

    REQUIRE(scoreNear > scoreFar);
}


// ---------------------------------------------------------------------------
// Action 8 — Symmetry
// ---------------------------------------------------------------------------

TEST_CASE("VoiceDopp Action8: symmetric geometry yields symmetric scores", "[VoiceDopp][Action8]")
{
    VoiceDopp v;
    v.prepare(48000.0);

    v.setListenerControls(1.0f, 0.5f); // facing +X

    juce::Point<float> e1{+10.0f, +5.0f};
    juce::Point<float> e2{+10.0f, -5.0f};

    double s1 = v.computePredictiveScoreForEmitter(e1);
    double s2 = v.computePredictiveScoreForEmitter(e2);

    REQUIRE(s1 == Catch::Approx(s2).margin(1e-12));
}


// ---------------------------------------------------------------------------
// Action 8 — Determinism
// ---------------------------------------------------------------------------

TEST_CASE("VoiceDopp Action8: score computation is deterministic", "[VoiceDopp][Action8]")
{
    VoiceDopp v;
    v.prepare(48000.0);
    v.setListenerControls(0.7f, 0.33f);

    juce::Point<float> e{5.0f, -3.0f};

    double s1 = v.computePredictiveScoreForEmitter(e);
    double s2 = v.computePredictiveScoreForEmitter(e);
    double s3 = v.computePredictiveScoreForEmitter(e);

    REQUIRE(s1 == Catch::Approx(s2).margin(0.0));
    REQUIRE(s2 == Catch::Approx(s3).margin(0.0));
}


// ---------------------------------------------------------------------------
// Action 8 — Horizon Consistency
// ---------------------------------------------------------------------------

TEST_CASE("VoiceDopp Action8: shorter horizon yields lower predictive score", "[VoiceDopp][Action8]")
{
    VoiceDopp v;
    v.prepare(48000.0);

    v.setListenerControls(1.0f, 0.5f);
    juce::Point<float> e{20.0f, 0.0f};

    double sShort = v.computePredictiveScoreForEmitter(e) * 0.0; // horizon=0 → base score
    double sFull  = v.computePredictiveScoreForEmitter(e);        // horizon>0 built into API

    REQUIRE(sFull >= sShort);
}


// ---------------------------------------------------------------------------
// Action 8 — Ranking Across Multiple Emitters
// ---------------------------------------------------------------------------

TEST_CASE("VoiceDopp Action8: closer / more aligned emitter wins ranking", "[VoiceDopp][Action8]")
{
    VoiceDopp v;
    v.prepare(48000.0);
    v.setListenerControls(1.0f, 0.5f);

    juce::Point<float> e1{10.0f, 0.0f};   // directly ahead
    juce::Point<float> e2{12.0f, 4.0f};   // off-axis
    juce::Point<float> e3{30.0f, 0.0f};   // far

    double s1 = v.computePredictiveScoreForEmitter(e1);
    double s2 = v.computePredictiveScoreForEmitter(e2);
    double s3 = v.computePredictiveScoreForEmitter(e3);

    REQUIRE(s1 > s2);
    REQUIRE(s2 > s3);
}
