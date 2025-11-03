#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <juce_dsp/juce_dsp.h>

TEST_CASE("SmoothedValue ramps correctly (in-progress)") {
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> s;
    s.reset(48000, 0.01);
    s.setCurrentAndTargetValue(0.0f);
    s.setTargetValue(1.0f);

    float x0 = s.getNextValue();
    for (int i = 0; i < 479; ++i)
        s.getNextValue(); // just shy of full duration

    float x1 = s.getCurrentValue();
    REQUIRE(x1 > x0);
    REQUIRE(x1 <= 1.0f);
}
