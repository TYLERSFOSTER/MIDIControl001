#pragma once

#include "dsp/BaseVoice.h"
#include "params/ParameterSnapshot.h"

#include <cmath>
#include <atomic>

// We need juce::Point; pull in the graphics module.
#include <juce_graphics/juce_graphics.h>

// ============================================================
// VoiceDopp — Phase IV Action 1  (Kinematic API Only)
// Action 2 code may exist, but MUST remain disabled.
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
        headingNorm_ = 0.5f;

        // Action-2 feature gate MUST remain off for Action-1 tests
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
        // keep stored controls
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
        // Action-2: time accumulator (DISABLED)
        // ======================================
        if (enableTimeAccumulation_)
        {
            if (sampleRate_ > 0.0)
            {
                double dt = 1.0 / sampleRate_;
                timeSec_ += static_cast<double>(numSamples) * dt;
            }
        }

        // Still silent (Actions 1–2)
        std::fill(buffer, buffer + numSamples, 0.0f);
    }

    // ------------------------------------------------------------
    // Action-1 Kinematic API
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
    // Action-2 control (tests will call this later)
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

    // Action-2 feature flag (MUST remain false for Action-1 tests)
    bool               enableTimeAccumulation_ = false;
};
