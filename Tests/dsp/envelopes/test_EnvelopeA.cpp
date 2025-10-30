#include <catch2/catch_test_macros.hpp>
#include "dsp/envelopes/EnvelopeA.h"

TEST_CASE("EnvelopeA basic attack-release behavior", "[envelope]") {
    EnvelopeA env;
    env.prepare(1000.0);
    env.setAttack(0.01f);   // 10 ms
    env.setRelease(0.01f);

    // Attack phase should rise to 1
    env.noteOn();
    float last = 0.0f;
    for (int i = 0; i < 20; ++i) {
        float next = env.nextSample();
        REQUIRE(next >= last);
        last = next;
    }

    // Now release should decay to 0
    env.noteOff();
    last = 1.0f;
    for (int i = 0; i < 20; ++i) {
        float next = env.nextSample();
        REQUIRE(next <= last);
        last = next;
    }

    REQUIRE_FALSE(env.isActive());
}
