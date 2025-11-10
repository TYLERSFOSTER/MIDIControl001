#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
using Catch::Approx;

#include "utils/dsp_metrics.h"
#include "dsp/VoiceManager.h"
#include "dsp/PeakGuard.h"
#include <filesystem>

// ============================================================
// VoiceManager integration tests — Step 15 with PeakGuard
// ============================================================

TEST_CASE("VoiceManager basic polyphony", "[voicemanager]") {
    VoiceManager mgr([] { return ParameterSnapshot{}; });
    ParameterSnapshot snap;
    snap.oscFreq = 220.0f;
    snap.envAttack = 0.001f;
    snap.envRelease = 0.05f;

    mgr.prepare(44100.0);
    mgr.startBlock();

    // trigger two notes
    mgr.handleNoteOn(60, 1.0f);
    mgr.handleNoteOn(64, 1.0f);

    std::vector<float> buffer(256, 0.0f);
    mgr.render(buffer.data(), (int)buffer.size());

    // buffer should contain nonzero samples
    REQUIRE(std::any_of(buffer.begin(), buffer.end(),
                        [](float x) { return std::fabs(x) > 0.0f; }));

    auto hash = hashBuffer(buffer);
    auto rms  = computeRMS(buffer);
    auto peak = computePeak(buffer);
    std::cout << "[DEBUG] calling writeJson()\n";
    namespace fs = std::filesystem;
    auto jsonPath = fs::path(__FILE__).parent_path()
                    / ".." / "baseline" / "voice_output_reference.json";
    writeJson(jsonPath.string(), hash, rms, peak);

    // ensure limiter never lets samples exceed hard limit
    REQUIRE(peak <= Approx(1.0f).margin(0.01f));

    // trigger note offs
    mgr.handleNoteOff(60);
    mgr.handleNoteOff(64);

    // let envelopes decay
    for (int i = 0; i < 4410; ++i) {
        float tmp[1] = {0.0f};
        mgr.render(tmp, 1);
    }

    // everything should be released
    std::vector<float> silent(128, 0.0f);
    mgr.render(silent.data(), (int)silent.size());
    bool allSilent = std::all_of(silent.begin(), silent.end(),
                                 [](float x) { return std::fabs(x) < 1e-5f; });
    REQUIRE(allSilent);
}

TEST_CASE("VoiceManager voice stealing", "[voicemanager]") {
    VoiceManager mgr([] { return ParameterSnapshot{}; });
    ParameterSnapshot snap;
    mgr.prepare(44100.0);
    mgr.startBlock();

    // fill all voices
    for (int i = 0; i < VoiceManager::maxVoices; ++i)
        mgr.handleNoteOn(40 + i, 1.0f);

    // add one more note — should steal quietest
    mgr.handleNoteOn(100, 1.0f);

    std::vector<float> buf(64, 0.0f);
    mgr.render(buf.data(), (int)buf.size());

    REQUIRE(std::any_of(buf.begin(), buf.end(),
                        [](float x) { return std::fabs(x) > 0.0f; }));

    // Verify limiter remains idle for moderate signals
    PeakGuard pg;
    pg.prepare(44100.0);
    float idle = 0.25f;
    float limited = pg.process(idle);
    REQUIRE(limited == Approx(idle).margin(0.05f));
}
