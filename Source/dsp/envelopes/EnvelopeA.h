#pragma once
#include <algorithm>

class EnvelopeA {
public:
    void prepare(double sampleRate);
    void setAttack(float seconds);
    void setRelease(float seconds);

    void noteOn();
    void noteOff();

    float nextSample();               // amplitude for next sample
    bool  isActive() const;           // false once fully released

    // ============================================================
    // Diagnostic / read-only accessor (added safely)
    // ============================================================
    float getCurrentValue() const noexcept { return static_cast<float>(level_); }

private:
    enum class State { Idle, Attack, Sustain, Release };
    State  state_ = State::Idle;
    double sampleRate_ = 44100.0;
    double level_ = 0.0;
    double attackInc_ = 0.0;
    double releaseCoef_ = 0.0;   // multiplicative decay per-sample

    double releaseStartLevel_ = 0.0;
    uint64_t releaseSamples_ = 0;
    double releaseSeconds_ = 0.2;     // store user-set release time
};
