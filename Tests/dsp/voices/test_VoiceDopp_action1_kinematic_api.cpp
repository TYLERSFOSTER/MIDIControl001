#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
using Catch::Approx;

#include "dsp/voices/VoiceDopp.h"
#include "params/ParameterSnapshot.h"

// ---------------------------------------------------------------------------
// Phase IV — Action 1
// Test: Kinematic API exposed and inert (no integration, no time advance)
// ---------------------------------------------------------------------------
TEST_CASE("VoiceDopp Action1: Kinematic API is present and inert",
          "[voice][dopp][action1][api][inert]")
{
    VoiceDopp v;

    const double sampleRate = 48000.0;
    v.prepare(sampleRate);

    ParameterSnapshot snap;
    v.noteOn(snap, 60, 0.8f);

    // Apply listener controls (stored only, inert)
    v.setListenerControls(1.0f, 0.5f);

    // Render 1 second of silence
    const int numSamples = 48000;
    std::vector<float> buf(numSamples, 0.0f);
    v.render(buf.data(), numSamples);

    // -- No time should have elapsed
    REQUIRE(v.getListenerTimeSeconds() == Approx(0.0).margin(1e-12));

    // -- Position should still be origin
    auto pos = v.getListenerPosition();
    REQUIRE(pos.x == Approx(0.0f).margin(1e-12f));
    REQUIRE(pos.y == Approx(0.0f).margin(1e-12f));

    // -- Voice is still alive
    REQUIRE(v.isActive() == true);

    // -- Output remains silent
    for (float s : buf)
        REQUIRE(s == Approx(0.0f));
}
