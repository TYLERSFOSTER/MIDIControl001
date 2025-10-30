#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
using Catch::Approx;

#include "dsp/Voice.h"

TEST_CASE("Voice basic lifecycle", "[voice]") {
    Voice v;
    ParameterSnapshot snap;
    snap.oscFreq   = 220.0f;
    snap.envAttack = 0.001f;
    snap.envRelease = 0.01f;

    v.prepare(44100.0);
    REQUIRE_FALSE(v.isActive());

    v.noteOn(snap, 57, 1.0f);
    REQUIRE(v.isActive());

    std::vector<float> buffer(128, 0.0f);
    v.render(buffer.data(), (int)buffer.size());
    REQUIRE(v.isActive());
    REQUIRE(std::any_of(buffer.begin(), buffer.end(),
                        [](float x) { return std::fabs(x) > 0.0f; }));

    v.noteOff();
    for (int i = 0; i < 4410; ++i) { // ~0.1 sec fade
        float tmp[1] = {0.0f};
        v.render(tmp, 1);
    }
    REQUIRE_FALSE(v.isActive());
}

TEST_CASE("Voice tracks level", "[voice]") {
    Voice v;
    ParameterSnapshot snap;
    v.prepare(44100.0);
    v.noteOn(snap, 69, 1.0f);
    std::vector<float> buf(32, 0.0f);
    v.render(buf.data(), 32);
    REQUIRE(v.getCurrentLevel() >= 0.0f);
}
