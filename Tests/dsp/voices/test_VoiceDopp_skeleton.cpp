#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "dsp/voices/VoiceDopp.h"
#include <vector>

TEST_CASE("VoiceDopp skeleton: basic lifecycle", "[voice][dopp]")
{
    VoiceDopp v;
    v.prepare(48000.0);

    REQUIRE(v.isActive() == false);
    REQUIRE(v.getNote() == -1);

    ParameterSnapshot snap;
    v.noteOn(snap, 60, 0.8f);

    REQUIRE(v.isActive() == true);
    REQUIRE(v.getNote() == 60);
    REQUIRE(v.getCurrentLevel() > 0.0f);

    std::vector<float> buf(256);
    v.render(buf.data(), 256);

    // Skeleton produces silence
    for (float x : buf)
        REQUIRE(x == Catch::Approx(0.0f));

    v.noteOff();
    REQUIRE(v.isActive() == false);
}
