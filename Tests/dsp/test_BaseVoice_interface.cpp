#include <catch2/catch_test_macros.hpp>
#include "dsp/BaseVoice.h"
#include "dsp/voices/VoiceA.h"

TEST_CASE("BaseVoice interface compiles", "[basevoice]") {
    BaseVoice* v = new VoiceA();
    v->prepare(44100.0);
    v->noteOn(ParameterSnapshot{}, 60, 1.0f);
    v->noteOff();
    float buf[1] = {0.0f};
    v->render(buf, 1);
    bool active = v->isActive();
    int note = v->getNote();
    float level = v->getCurrentLevel();
    delete v;

    (void)active;
    (void)note;
    (void)level;
    REQUIRE(true);
}
