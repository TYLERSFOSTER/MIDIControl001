#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "dsp/voices/VoiceDopp.h"
#include <cmath>
#include <limits>

TEST_CASE("VoiceDopp Action7: carrier at retarded time has correct frequency",
          "[VoiceDopp][Action7]")
{
    VoiceDopp v;
    v.prepare(48000.0);

    // Set base frequency to a simple value we can reason about
    const double freq = 440.0;
    v.setBaseFrequencyForTest(freq);

    const double T = 1.0 / freq;  // one period

    double s0 = v.evalCarrierAtRetardedTime(0.0);
    double s1 = v.evalCarrierAtRetardedTime(T);

    // After one full period, the sinusoid must return to the same value.
    REQUIRE(s0 == Catch::Approx(s1).margin(1e-9));
}

TEST_CASE("VoiceDopp Action7: field pulse matches analytic sinusoidal envelope",
          "[VoiceDopp][Action7]")
{
    VoiceDopp v;
    v.prepare(48000.0);

    const double mu = 2.0; // 2 Hz pulse
    v.setFieldPulseFrequencyForTest(mu);

    // A_field(t) = 0.5 * (1 + sin(2π μ t))

    double t0   = 0.0;
    double tQ   = 1.0 / (4.0 * mu);   // quarter period
    double tH   = 1.0 / (2.0 * mu);   // half period
    double t3Q  = 3.0 / (4.0 * mu);   // three quarters

    auto A = [&](double t) {
        return 0.5 * (1.0 + std::sin(2.0 * M_PI * mu * t));
    };

    REQUIRE(v.evalFieldPulseAtRetardedTime(t0)  == Catch::Approx(A(t0)).margin(1e-9));
    REQUIRE(v.evalFieldPulseAtRetardedTime(tQ)  == Catch::Approx(A(tQ)).margin(1e-9));
    REQUIRE(v.evalFieldPulseAtRetardedTime(tH)  == Catch::Approx(A(tH)).margin(1e-9));
    REQUIRE(v.evalFieldPulseAtRetardedTime(t3Q) == Catch::Approx(A(t3Q)).margin(1e-9));
}

TEST_CASE("VoiceDopp Action7: ADSR envelope attack/decay/sustain segments",
          "[VoiceDopp][Action7]")
{
    VoiceDopp v;
    v.prepare(48000.0);

    const double attack  = 0.1;
    const double decay   = 0.1;
    const double sustain = 0.5;
    const double release = 0.2;

    v.setAdsrParamsForTest(attack, decay, sustain, release);

    // Note-on at t_on = 0, no release (t_off = +∞)
    v.setAdsrTimesForTest(0.0, std::numeric_limits<double>::infinity());

    // Before note-on: envelope must be 0
    REQUIRE(v.evalAdsrAtRetardedTime(-0.01) == Catch::Approx(0.0).margin(1e-9));
    REQUIRE(v.evalAdsrAtRetardedTime(0.0)   == Catch::Approx(0.0).margin(1e-9));

    // Mid-attack: t = 0.5 * attack → env = 0.5
    double tAttackMid = 0.5 * attack;
    REQUIRE(v.evalAdsrAtRetardedTime(tAttackMid)
            == Catch::Approx(0.5).margin(1e-6));

    // End of attack: t = attack → env ≈ 1.0
    REQUIRE(v.evalAdsrAtRetardedTime(attack)
            == Catch::Approx(1.0).margin(1e-6));

    // Mid-decay: halfway from 1 → sustain
    double tDecayMid = attack + 0.5 * decay;
    double expectedDecayMid = 1.0 + (sustain - 1.0) * 0.5; // linear halfway
    REQUIRE(v.evalAdsrAtRetardedTime(tDecayMid)
            == Catch::Approx(expectedDecayMid).margin(1e-6));

    // Well after decay: should be at sustain level
    double tSustain = attack + decay + 0.1;
    REQUIRE(v.evalAdsrAtRetardedTime(tSustain)
            == Catch::Approx(sustain).margin(1e-6));
}

TEST_CASE("VoiceDopp Action7: ADSR envelope release segment at retarded time",
          "[VoiceDopp][Action7]")
{
    VoiceDopp v;
    v.prepare(48000.0);

    const double attack  = 0.1;
    const double decay   = 0.1;
    const double sustain = 0.5;
    const double release = 0.2;

    v.setAdsrParamsForTest(attack, decay, sustain, release);

    // Note-on at t_on = 0, note-off at t_off = 0.3 (after A+D)
    const double tOn  = 0.0;
    const double tOff = 0.3;
    v.setAdsrTimesForTest(tOn, tOff);

    // At t_ret = t_off, we should already be in sustain
    double envAtReleaseStart = v.evalAdsrAtRetardedTime(tOff);
    REQUIRE(envAtReleaseStart == Catch::Approx(sustain).margin(1e-6));

    // Halfway through release: t = t_off + release/2 → env ≈ sustain / 2
    double tMidRelease = tOff + 0.5 * release;
    double envMid = v.evalAdsrAtRetardedTime(tMidRelease);
    REQUIRE(envMid == Catch::Approx(sustain * 0.5).margin(1e-6));

    // After full release: t >= t_off + release → 0
    double tAfterRelease = tOff + release + 0.05;
    REQUIRE(v.evalAdsrAtRetardedTime(tAfterRelease)
            == Catch::Approx(0.0).margin(1e-9));
}
