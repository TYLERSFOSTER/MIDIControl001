#include <catch2/catch_test_macros.hpp>
#include "dsp/InstrumentRegistry.h"

TEST_CASE("InstrumentRegistry creates VoiceA", "[registry]")
{
    auto& reg = InstrumentRegistry::instance();
    auto voice = reg.makeVoice("voiceA");
    REQUIRE(voice != nullptr);
    REQUIRE_FALSE(voice->isActive());

    voice->prepare(44100.0);
    ParameterSnapshot snap;
    snap.oscFreq = 440.0f;
    snap.envAttack = 0.001f;
    snap.envRelease = 0.05f;

    voice->noteOn(snap, 69, 1.0f);
    REQUIRE(voice->isActive());

    std::vector<float> buf(64, 0.0f);
    voice->render(buf.data(), 64);
    REQUIRE(std::any_of(buf.begin(), buf.end(),
                        [](float x){ return std::fabs(x) > 0.0f; }));
}
