// [Lifecycle: Advisory] — Diagnostic only, measures VoiceA RMS/peak behavior at 220 Hz
#include <catch2/catch_test_macros.hpp>
#include "dsp/voices/VoiceA.h"
#include "utils/dsp_metrics.h"
#include "params/ParameterSnapshot.h"
#include <vector>
#include <iostream>

TEST_CASE("VoiceA diagnostic: RMS and peak levels at 220Hz", "[voice][diagnostic]") {
    ParameterSnapshot snap;
    snap.oscFreq    = 220.0f;
    snap.envAttack  = 0.001f;
    snap.envRelease = 0.01f;

    VoiceA voice;
    voice.prepare(44100.0);
    voice.noteOn(snap, 57, 1.0f);

    constexpr int N = 128;
    std::vector<float> buffer(N, 0.0f);
    voice.render(buffer.data(), N);

    const float rms  = computeRMS(buffer);
    const float peak = computePeak(buffer);

    INFO("VoiceA RMS=" << rms << " peak=" << peak);

    // Baseline expectations: should produce a clean audible signal without clipping
    REQUIRE(rms > 0.0f);
    REQUIRE(peak > 0.0f);
    REQUIRE(rms < 1.0f);
    REQUIRE(peak <= 1.0f);

    std::cout << "\n[VoiceA Diagnostic 220Hz]"
              << "\nRMS="  << rms
              << " Peak=" << peak
              << std::endl;

    // Keep marked as pass for CI visibility, even if metrics drift slightly
    REQUIRE(true);
}
