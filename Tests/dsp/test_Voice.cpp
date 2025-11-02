#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
using Catch::Approx;

#include "dsp/voices/VoiceA.h"
#include "params/ParameterSnapshot.h"
#include <vector>
#include <algorithm>
#include <cmath>

TEST_CASE("Voice lifecycle basic", "[voice][lifecycle]")
{
    VoiceA v;
    ParameterSnapshot snap;
    snap.oscFreq    = 220.0f;
    snap.envAttack  = 0.001f;
    snap.envRelease = 0.01f;

    v.prepare(44100.0);
    REQUIRE_FALSE(v.isActive());

    v.noteOn(snap, 57, 1.0f);
    REQUIRE(v.isActive());

    // --- warmup blocks to let smoothing settle ---
    std::vector<float> buffer(128, 0.0f);
    for (int i = 0; i < 3; ++i)
        v.render(buffer.data(), (int)buffer.size());

    // --- test steady-state render ---
    std::fill(buffer.begin(), buffer.end(), 0.0f);
    v.render(buffer.data(), (int)buffer.size());

    // --- verify voice produced a valid signal ---
    float sumSq = 0.0f, peak = 0.0f;
    for (float x : buffer)
    {
        sumSq += x * x;
        peak = std::max(peak, std::fabs(x));
    }
    float rms = std::sqrt(sumSq / buffer.size());

    // Log for visibility in CI/test reports
    INFO("VoiceA render metrics: RMS=" << rms << "  Peak=" << peak);

    REQUIRE(rms > 0.1f);   // not silent
    REQUIRE(peak > 0.2f);  // audible amplitude

    v.noteOff();

    // --- simulate release phase (~0.1s) ---
    for (int i = 0; i < 4410; ++i)
    {
        float tmp[1] = {0.0f};
        v.render(tmp, 1);
    }
    REQUIRE_FALSE(v.isActive());
}

TEST_CASE("Voice lifecycle basic tracks level", "[voice][lifecycle]")
{
    VoiceA v;
    ParameterSnapshot snap;
    v.prepare(44100.0);
    v.noteOn(snap, 69, 1.0f);

    std::vector<float> buf(32, 0.0f);
    v.render(buf.data(), 32);

    REQUIRE(v.getCurrentLevel() >= 0.0f);
}
