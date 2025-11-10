#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>   // explicitly include for Catch::Approx
#include <juce_audio_basics/juce_audio_basics.h>
#include "dsp/MixNormalizer.h"

TEST_CASE("MixNormalizer basic lifecycle", "[dsp][MixNormalizer]")
{
    MixNormalizer norm;
    juce::AudioBuffer<float> buffer(2, 128);
    buffer.clear();

    norm.prepare(44100.0, 128);
    norm.process(buffer);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            REQUIRE(buffer.getSample(ch, i) == 0.0f);

    REQUIRE(norm.getLastGain() == Catch::Approx(1.0f));
}
