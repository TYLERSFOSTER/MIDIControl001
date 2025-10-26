#include <catch2/catch_test_macros.hpp>
#include <juce_dsp/juce_dsp.h>

TEST_CASE("Smoothing sanity") {
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> s;
    s.reset(48000, 0.01);
    s.setCurrentAndTargetValue(0.0f);
    s.setTargetValue(1.0f);
    float x0 = s.getNextValue();
    for (int i = 0; i < 480; ++i) s.getNextValue(); // 10ms @ 48k
    float x1 = s.getCurrentValue();
    REQUIRE(x1 > x0);
    REQUIRE(x1 < 1.0f);
}
