#pragma once
#include <juce_audio_basics/juce_audio_basics.h>

/**
 * MixNormalizer — Phase II Step 16 baseline
 *
 * Purpose:
 *   Placeholder for post-mix normalization stage.
 *   Will later scale mixed audio so |sample| ≤ 1.0 without perceptible pumping.
 *
 * Current behavior:
 *   Pass-through (no gain change).
 */
class MixNormalizer
{
public:
    MixNormalizer() = default;

    /** Prepare for a given sample rate and block size. */
    void prepare(double newSampleRate, int newBlockSize)
    {
        sampleRate = newSampleRate;
        blockSize  = newBlockSize;
    }

    /** Reset internal state. */
    void reset() {}

    /**
     * Process in place.
     * @param buffer  Audio buffer to normalize.
     */
    void process(juce::AudioBuffer<float>& buffer)
    {
        juce::ignoreUnused(buffer);
        // baseline: pass-through
    }

    /** Get current normalization gain (placeholder). */
    float getLastGain() const noexcept { return 1.0f; }

private:
    double sampleRate {44100.0};
    int blockSize {512};
};
