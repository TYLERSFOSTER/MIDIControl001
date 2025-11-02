#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
using Catch::Approx;

#include "dsp/voices/VoiceA.h"
#include "params/ParameterSnapshot.h"
#include <vector>
#include <cmath>
#include <algorithm>

// ============================================================
// Voice scalecheck lifecycle — no baselines, frequency variant
// ============================================================
TEST_CASE("Voice scalecheck lifecycle", "[voice][scalecheck]") {
    VoiceA v;
    ParameterSnapshot snap;
    snap.oscFreq    = 220.0f;   // lower pitch, same envelope
    snap.envAttack  = 0.001f;
    snap.envRelease = 0.02f;

    v.prepare(44100.0);
    REQUIRE_FALSE(v.isActive());

    v.noteOn(snap, 57, 1.0f);
    REQUIRE(v.isActive());

    // Warm-up: let attack ramp settle
    std::vector<float> warm(256, 0.0f);
    for (int i = 0; i < 3; ++i)
        v.render(warm.data(), (int)warm.size());

    // Main measurement buffer
    std::vector<float> buffer(256, 0.0f);
    v.render(buffer.data(), (int)buffer.size());
    REQUIRE(v.isActive());

    // Compute metrics
    float sumSq = 0.0f, peak = 0.0f;
    for (float x : buffer) {
        sumSq += x * x;
        peak = std::max(peak, std::fabs(x));
    }
    float rms = std::sqrt(sumSq / buffer.size());

    INFO("VoiceA scalecheck: freq=" << snap.oscFreq
         << " RMS=" << rms
         << " Peak=" << peak);

    // Physical sanity checks
    REQUIRE(rms > 0.05f);  // audible signal present
    REQUIRE(peak > 0.1f);
    REQUIRE(rms < 1.0f);   // not clipped

    v.noteOff();
    for (int i = 0; i < 4410; ++i) {
        float tmp[1] = {0.0f};
        v.render(tmp, 1);
    }
    REQUIRE_FALSE(v.isActive());
}

// ============================================================
// Voice scalecheck tracks level — diagnostic only
// ============================================================
TEST_CASE("Voice scalecheck tracks level", "[voice][scalecheck]") {
    VoiceA v;
    ParameterSnapshot snap;
    v.prepare(44100.0);
    v.noteOn(snap, 69, 1.0f);
    std::vector<float> buf(32, 0.0f);
    v.render(buf.data(), 32);
    INFO("VoiceA current level: " << v.getCurrentLevel());
    REQUIRE(v.getCurrentLevel() >= 0.0f);
}
