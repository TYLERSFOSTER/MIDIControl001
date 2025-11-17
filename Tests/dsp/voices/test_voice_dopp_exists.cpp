#include <catch2/catch_test_macros.hpp>
#include "dsp/voices/VoiceDopp.h"
#include "dsp/BaseVoice.h"

TEST_CASE("VoiceDopp exists and is a BaseVoice", "[voice][dopp][A]")
{
    VoiceDopp v;

    // Compile-time guarantee: VoiceDopp is-a BaseVoice
    BaseVoice* basePtr = &v;
    REQUIRE(basePtr != nullptr);

    // Optional: ensure required virtuals exist
    REQUIRE_NOTHROW(v.prepare(48000.0));
    REQUIRE_NOTHROW(v.noteOn(ParameterSnapshot{}, 60, 0.8f));
    REQUIRE_NOTHROW(v.noteOff());
    REQUIRE_NOTHROW(v.render(nullptr, 0));
}
