#include <catch2/catch_test_macros.hpp>
#include "dsp/legacy/VoiceLegacy.h"
#include "dsp/voices/VoiceA.h"
#include "utils/dsp_metrics.h"
#include "params/ParameterSnapshot.h"
#include <vector>
#include <iostream>

TEST_CASE("Voice diagnostic: baseline vs VoiceA at 220Hz", "[voice][diagnostic]") {
    ParameterSnapshot snap;
    snap.oscFreq    = 220.0f;
    snap.envAttack  = 0.001f;
    snap.envRelease = 0.01f;

    VoiceLegacy legacy;
    VoiceA      modern;
    legacy.prepare(44100.0);
    modern.prepare(44100.0);

    legacy.noteOn(snap, 57, 1.0f);
    modern.noteOn(snap, 57, 1.0f);

    constexpr int N = 128;
    std::vector<float> bufLegacy(N, 0.0f);
    std::vector<float> bufModern(N, 0.0f);
    legacy.render(bufLegacy.data(), N);
    modern.render(bufModern.data(), N);

    float rmsL = computeRMS(bufLegacy);
    float peakL = computePeak(bufLegacy);
    float rmsM = computeRMS(bufModern);
    float peakM = computePeak(bufModern);

    INFO("Legacy RMS=" << rmsL << " peak=" << peakL);
    INFO("Modern RMS=" << rmsM << " peak=" << peakM);

    // Compare ratios
    INFO("RMS ratio (modern/legacy) = " << (rmsM / rmsL));
    INFO("Peak ratio (modern/legacy) = " << (peakM / peakL));

    // Allow wide tolerance just to confirm directionality
    REQUIRE(rmsL > 0.0f);
    REQUIRE(peakL > 0.0f);
    REQUIRE(rmsM > 0.0f);
    REQUIRE(peakM > 0.0f);

    INFO("Legacy RMS=" << rmsL << " peak=" << peakL);
    INFO("Modern RMS=" << rmsM << " peak=" << peakM);
    INFO("RMS ratio (modern/legacy) = " << (rmsM / rmsL));
    INFO("Peak ratio (modern/legacy) = " << (peakM / peakL));

    // Always print results for debugging
    std::cout << "\n[Voice Diagnostic 220Hz]"
              << "\nLegacy: RMS=" << rmsL << " Peak=" << peakL
              << "\nModern: RMS=" << rmsM << " Peak=" << peakM
              << "\nRatios: RMS=" << (rmsM / rmsL)
              << " Peak=" << (peakM / peakL)
              << std::endl;

    REQUIRE(true); // keep test marked as pass
}
