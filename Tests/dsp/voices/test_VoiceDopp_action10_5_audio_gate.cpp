#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
using Catch::Approx;

#include "dsp/voices/VoiceDopp.h"
#include "params/ParameterSnapshot.h"
#include <vector>

TEST_CASE("VoiceDopp A10-5: audioEnabled=false keeps render silent", "[VoiceDopp][A10-5]")
{
    VoiceDopp v;
    v.prepare(48000.0);

    ParameterSnapshot s;
    s.oscFreq    = 220.0;
    s.envAttack  = 0.01;
    s.envRelease = 0.2;

    v.noteOn(s, 60, 1.0f);

    v.enableTimeAccumulation(true);
    v.setAudioSynthesisEnabled(false);

    std::vector<float> buf(256, 123.0f);
    v.render(buf.data(), (int)buf.size());

    for (auto x : buf)
        REQUIRE(x == Approx(0.0f));
}

TEST_CASE("VoiceDopp A10-5: audioEnabled=true produces nonzero samples", "[VoiceDopp][A10-5]")
{
    VoiceDopp v;
    v.prepare(48000.0);

    ParameterSnapshot s;
    s.oscFreq    = 220.0;
    s.envAttack  = 0.01;
    s.envRelease = 0.2;

    v.noteOn(s, 60, 1.0f);

    v.enableTimeAccumulation(true);
    v.setListenerControls(0.5f, 0.5f); // some speed, heading 0
    v.setEmitterFieldControls(0.5f, 0.0f);
    v.setAudioSynthesisEnabled(true);

    std::vector<float> buf(512, 0.0f);
    v.render(buf.data(), (int)buf.size());

    bool anyNonZero = false;
    for (auto x : buf)
        if (std::abs(x) > 1e-6f) { anyNonZero = true; break; }

    REQUIRE(anyNonZero);
}
