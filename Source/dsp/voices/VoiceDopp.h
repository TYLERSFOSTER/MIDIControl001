#pragma once

#include "dsp/BaseVoice.h"
#include "params/ParameterSnapshot.h"

#include <cmath>
#include <atomic>

// We need juce::Point; pull in the graphics module.
#include <juce_graphics/juce_graphics.h>

// ============================================================
// VoiceDopp — Phase IV Action 1–3.1
// ------------------------------------------------------------
// Action 1: Kinematic API (position, time, listener controls)
// Action 2: Time accumulator (feature-gated, still OFF)
// Action 3.1: Heading + speed mapping (pure math only)
// ============================================================

class VoiceDopp : public BaseVoice
{
public:
    VoiceDopp()  = default;
    ~VoiceDopp() override = default;

    // ------------------------------------------------------------
    // prepare()
    // ------------------------------------------------------------
    void prepare(double sampleRate) override
    {
        sampleRate_ = sampleRate;
        active_     = false;
        midiNote_   = -1;
        level_      = 0.0f;

        // Kinematic state
        listenerPos_ = { 0.0f, 0.0f };
        timeSec_     = 0.0;
        speedNorm_   = 0.0f;
        headingNorm_ = 0.5f;   // θ = 0

        // Action-2 gate stays off by default
        enableTimeAccumulation_ = false;
    }

    // ------------------------------------------------------------
    // noteOn()
    // ------------------------------------------------------------
    void noteOn(const ParameterSnapshot& snapshot,
                int midiNote,
                float velocity) override
    {
        (void)snapshot;
        (void)velocity;

        midiNote_ = midiNote;
        active_   = true;
        level_    = 1.0f;

        listenerPos_ = { 0.0f, 0.0f };
        timeSec_     = 0.0;
    }

    // ------------------------------------------------------------
    // noteOff()
    // ------------------------------------------------------------
    void noteOff() override
    {
        active_ = false;
        level_  = 0.0f;
    }

    // ------------------------------------------------------------
    // render()
    // Action-1: silent + inert
    // Action-2: time accumulation (guarded)
    // ------------------------------------------------------------
    void render(float* buffer, int numSamples) override
    {
        if (!active_)
        {
            std::fill(buffer, buffer + numSamples, 0.0f);
            return;
        }

        // ======================================
        // Action-2 time accumulator (still gated)
        // ======================================
        if (enableTimeAccumulation_)
        {
            if (sampleRate_ > 0.0)
            {
                double dt = 1.0 / sampleRate_;
                timeSec_ += static_cast<double>(numSamples) * dt;
            }
        }

        // Still silent
        std::fill(buffer, buffer + numSamples, 0.0f);
    }

    // ------------------------------------------------------------
    // Action-1: Kinematic API
    // ------------------------------------------------------------
    void setListenerControls(float speedNorm, float headingNorm)
    {
        speedNorm_   = speedNorm;
        headingNorm_ = headingNorm;
    }

    juce::Point<float> getListenerPosition() const
    {
        return listenerPos_;
    }

    double getListenerTimeSeconds() const
    {
        return timeSec_;
    }

    // ------------------------------------------------------------
    // Action-3.1: Pure mapping functions (NO behaviour change)
    // ------------------------------------------------------------
    double computeHeadingAngle() const
    {
        // θ = 2π * headingNorm − π
        return (2.0 * M_PI * static_cast<double>(headingNorm_)) - M_PI;
    }

    double computeSpeed() const
    {
        // v = v_max * speedNorm
        return vMax_ * static_cast<double>(speedNorm_);
    }

    juce::Point<float> computeUnitVector() const
    {
        double theta = computeHeadingAngle();
        return {
            static_cast<float>(std::cos(theta)),
            static_cast<float>(std::sin(theta))
        };
    }

    // ------------------------------------------------------------
    // Action-2 control (tests use this explicitly)
    // ------------------------------------------------------------
    void enableTimeAccumulation(bool shouldEnable)
    {
        enableTimeAccumulation_ = shouldEnable;
    }

    // ------------------------------------------------------------
    // State queries
    // ------------------------------------------------------------
    bool  isActive() const override         { return active_; }
    int   getNote() const noexcept override { return midiNote_; }
    float getCurrentLevel() const override  { return level_; }

private:
    // Phase III skeleton state
    double sampleRate_ = 48000.0;
    bool   active_     = false;
    int    midiNote_   = -1;
    float  level_      = 0.0f;

    // Action-1 state
    juce::Point<float> listenerPos_;
    double             timeSec_;
    float              speedNorm_;
    float              headingNorm_;

    // Action-2 flag
    bool enableTimeAccumulation_ = false;

    // ------------------------------------------------------------
    // Action-3.1 constant
    // ------------------------------------------------------------
    static constexpr double vMax_ = 1.0;
};
