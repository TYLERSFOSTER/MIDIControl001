#include <catch2/catch_all.hpp>
#include <catch2/catch_approx.hpp>
using Catch::Approx;

#include "dsp/voices/VoiceDopp.h"
#include "params/ParameterSnapshot.h"

// ------------------------------------------------------------
// Helpers
// ------------------------------------------------------------
static VoiceParams makeVP(double freq, double atk, double rel)
{
    VoiceParams vp;
    vp.oscFreq    = freq;
    vp.envAttack  = atk;
    vp.envRelease = rel;
    return vp;
}

TEST_CASE("VoiceDopp A10-1: parameter pipeline exists and is inert")
{
    VoiceDopp v;
    v.prepare(48000.0);

    // ----------------------------------------
    // 1. After prepare(), defaults are intact
    // ----------------------------------------
    REQUIRE(v.getBaseFrequencyForTest() == Approx(220.0));
    REQUIRE(v.getAttackForTest()        == Approx(0.01));
    REQUIRE(v.getReleaseForTest()       == Approx(0.20));

    // ----------------------------------------
    // 2. noteOn latches snapshot parameters
    // ----------------------------------------
    ParameterSnapshot snap;
    snap.oscFreq    = 880.0;
    snap.envAttack  = 0.5;
    snap.envRelease = 1.25;

    v.noteOn(snap, 60, 1.0f);

    REQUIRE(v.getBaseFrequencyForTest() == Approx(880.0));
    REQUIRE(v.getAttackForTest()        == Approx(0.5));
    REQUIRE(v.getReleaseForTest()       == Approx(1.25));

    // ----------------------------------------
    // 3. updateParams mutates latched values
    // ----------------------------------------
    VoiceParams vp = makeVP(1234.0, 0.02, 0.99);
    v.updateParams(vp);

    REQUIRE(v.getBaseFrequencyForTest() == Approx(1234.0));
    REQUIRE(v.getAttackForTest()        == Approx(0.02));
    REQUIRE(v.getReleaseForTest()       == Approx(0.99));

    // ----------------------------------------
    // 4. render still produces *silence*
    // ----------------------------------------
    float buf[64];
    v.render(buf, 64);

    for (float x : buf)
        REQUIRE(x == Approx(0.0f));
}

TEST_CASE("VoiceDopp A10-1: updateParams can be called before noteOn")
{
    VoiceDopp v;
    v.prepare(48000.0);

    VoiceParams vp = makeVP(500.0, 0.005, 0.8);
    v.updateParams(vp);

    REQUIRE(v.getBaseFrequencyForTest() == Approx(500.0));
    REQUIRE(v.getAttackForTest()        == Approx(0.005));
    REQUIRE(v.getReleaseForTest()       == Approx(0.8));
}

TEST_CASE("VoiceDopp A10-1: noteOn overrides pre-updated params")
{
    VoiceDopp v;
    v.prepare(48000.0);

    // Pre-update
    VoiceParams vp = makeVP(999.0, 0.03, 0.07);
    v.updateParams(vp);

    // Snapshot overrides
    ParameterSnapshot snap;
    snap.oscFreq    = 440.0;
    snap.envAttack  = 0.1;
    snap.envRelease = 0.2;

    v.noteOn(snap, 64, 1.0f);

    REQUIRE(v.getBaseFrequencyForTest() == Approx(440.0));
    REQUIRE(v.getAttackForTest()        == Approx(0.1));
    REQUIRE(v.getReleaseForTest()       == Approx(0.2));
}
