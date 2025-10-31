#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
using Catch::Approx;

#include "utils/dsp_metrics.h"
#include <filesystem>
#include "dsp/voices/VoiceA.h"

// ============================================================================
// Scale-check version of the baseline lifecycle test
// ============================================================================
TEST_CASE("Voice scalecheck lifecycle", "[voice][scalecheck]") {
    VoiceA v;
    ParameterSnapshot snap;
    snap.oscFreq    = 220.0f;
    snap.envAttack  = 0.001f;
    snap.envRelease = 0.01f;

    v.prepare(44100.0);
    REQUIRE_FALSE(v.isActive());

    v.noteOn(snap, 57, 1.0f);
    REQUIRE(v.isActive());

    std::vector<float> buffer(128, 0.0f);
    v.render(buffer.data(), (int)buffer.size());
    for (auto& s : buffer)
        s *= 0.5f; // apply 0.5x scaling hypothesis
    REQUIRE(v.isActive());

    auto hash = hashBuffer(buffer);
    auto rms  = computeRMS(buffer);
    auto peak = computePeak(buffer);

    namespace fs = std::filesystem;
    auto jsonPath = fs::path(__FILE__).parent_path()
                    / ".." / "baseline" / "voice_output_reference.json";

    bool ok = compareWithBaseline(jsonPath.string(), hash, rms, peak);
    REQUIRE(ok);

    REQUIRE(std::any_of(buffer.begin(), buffer.end(),
                        [](float x) { return std::fabs(x) > 0.0f; }));

    v.noteOff();
    for (int i = 0; i < 4410; ++i) { // ~0.1 sec fade
        float tmp[1] = {0.0f};
        v.render(tmp, 1);
    }
    REQUIRE_FALSE(v.isActive());
}

// ============================================================================
// Scale-check version of the level-tracking test (unique name)
// ============================================================================
TEST_CASE("Voice scalecheck tracks level", "[voice][scalecheck]") {
    VoiceA v;
    ParameterSnapshot snap;
    v.prepare(44100.0);
    v.noteOn(snap, 69, 1.0f);
    std::vector<float> buf(32, 0.0f);
    v.render(buf.data(), 32);
    REQUIRE(v.getCurrentLevel() >= 0.0f);
}
