#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "dsp/oscillators/OscillatorA.h"

using Catch::Approx;

TEST_CASE("OscillatorA produces valid sine output", "[oscillator]")
{
    OscillatorA osc;
    osc.prepare(48000.0);
    osc.setFrequency(480.0f); // 1/100 of SR -> 480 samples per cycle

    const int samplesPerCycle = 100;
    float sum = 0.0f;
    for (int i = 0; i < samplesPerCycle; ++i)
        sum += osc.nextSample();

    // The sum over one cycle should be near zero for a symmetric waveform
    REQUIRE(sum == Approx(0.0f).margin(0.1f));
}

TEST_CASE("OscillatorA resets phase correctly", "[oscillator]")
{
    OscillatorA osc;
    osc.prepare(44100.0);
    osc.setFrequency(1000.0f);

    const float first = osc.nextSample();
    for (int i = 0; i < 100; ++i) osc.nextSample();
    osc.resetPhase();
    const float reset = osc.nextSample();

    REQUIRE(reset == Approx(first).margin(1e-6f));
}

TEST_CASE("OscillatorA frequency doubles output period halves", "[oscillator]")
{
    OscillatorA osc;
    osc.prepare(48000.0);

    osc.setFrequency(240.0f);
    const float v1 = osc.nextSample();
    int steps240 = 0;
    while (osc.nextSample() > v1 && ++steps240 < 48000) {}

    osc.resetPhase();
    osc.setFrequency(480.0f);
    const float v2 = osc.nextSample();
    int steps480 = 0;
    while (osc.nextSample() > v2 && ++steps480 < 48000) {}

    REQUIRE(steps480 == Approx(steps240 / 2.0).margin(1.0));
}
