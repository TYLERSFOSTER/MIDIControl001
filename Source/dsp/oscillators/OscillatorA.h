#pragma once
#include <cmath>
#include <numbers>  // for std::numbers::pi_v

class OscillatorA {
public:
    void prepare(double sampleRate)
    {
        sampleRate_ = sampleRate > 0.0 ? sampleRate : 44100.0;
        phase_ = 0.0;
    }

    void setFrequency(float hz) { freq_ = hz; }
    float getFrequency() const noexcept { return freq_; }

    void resetPhase() { phase_ = 0.0; }

    float nextSample()
    {
        if (freq_ <= 0.0f)
            return 0.0f;  // ✅ silence when frequency zeroed after release

        const double value = std::sin(phase_);
        phase_ += twoPi * freq_ / sampleRate_;
        if (phase_ >= twoPi)
            phase_ -= twoPi; // wraparound

        // ✅ safety clamp for denormals and near-zero bleed
        return std::abs(value) < 1e-8 ? 0.0f : static_cast<float>(value);
    }

private:
    static constexpr double twoPi = 6.283185307179586476925286766559;
    double sampleRate_ = 44100.0;
    float  freq_       = 440.0f;
    double phase_      = 0.0;
};
