#pragma once

#include "dsp/BaseVoice.h"
#include "params/ParameterSnapshot.h"

#include <cmath>
#include <atomic>

// We need juce::Point; pull in the graphics module (which defines it).
#include <juce_graphics/juce_graphics.h>

// ============================================================
// VoiceDopp — Phase IV Action 1 (Kinematic API Only)
// ------------------------------------------------------------
// This file intentionally contains ONLY:
//   • Phase III skeleton behaviour (silence, activation tracking)
//   • Minimal Action-1 API (time getter, position getter,
//                          listener-controls setter)
// Nothing else. No integration. No CC logic. No physics.
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

        // Action 1 state (does NOT do motion):
        listenerPos_ = { 0.0f, 0.0f }; // always (0,0) in Action 1
        timeSec_     = 0.0;            // stays 0.0 in Action 1
        speedNorm_   = 0.0f;           // just stored
        headingNorm_ = 0.5f;           // default: “+x” direction
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

        // Action-1 behaviour: reset kinematic state
        listenerPos_ = { 0.0f, 0.0f };
        timeSec_     = 0.0;
        // speedNorm_ / headingNorm_ are preserved as “controls”
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
    // render() — Phase III skeleton, silent output.
    // NO physics, NO kinematics, NO integration.
    // ------------------------------------------------------------
    void render(float* buffer, int numSamples) override
    {
        if (!active_)
        {
            std::fill(buffer, buffer + numSamples, 0.0f);
            return;
        }

        // Still silent in Action 1
        std::fill(buffer, buffer + numSamples, 0.0f);
    }

    // ------------------------------------------------------------
    // Action 1: Kinematic API
    //   • setListenerControls: writes speed / heading (no motion)
    //   • getListenerPosition: returns current position (always (0,0))
    //   • getListenerTimeSeconds: returns current time (always 0.0)
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
    // State queries (BaseVoice interface)
    // ------------------------------------------------------------
    bool  isActive() const override         { return active_; }
    int   getNote() const noexcept override { return midiNote_; }
    float getCurrentLevel() const override  { return level_; }

    // We inherit BaseVoice::handleController(int, float) unchanged
    // for Action 1 (no per-voice CC logic yet).

private:
    // Phase III skeleton state
    double sampleRate_ = 48000.0;
    bool   active_     = false;
    int    midiNote_   = -1;
    float  level_      = 0.0f;

    // ------------------------------------------------------------
    // Action-1 state (NO behaviour yet!)
    // ------------------------------------------------------------
    juce::Point<float> listenerPos_;   // always (0,0) in Action 1
    double             timeSec_;       // always 0.0 in Action 1
    float              speedNorm_;     // stored only
    float              headingNorm_;   // stored only
};
