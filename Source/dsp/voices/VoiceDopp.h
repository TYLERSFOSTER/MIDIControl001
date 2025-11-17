#pragma once
#include "dsp/BaseVoice.h"
#include <cmath>
#include <atomic>
// Required for juce::ignoreUnused when compiling tests
#include <juce_core/juce_core.h>

// ============================================================
// VoiceDopp (Phase III-A.1 Skeleton)
// ------------------------------------------------------------
// A safe, inert implementation that renders silence and tracks
// activation state. This is the correct scaffolding for future
// Doppler math (retarded time, lattice, predictive selection).
//
// The goal: compile, run, pass tests, never affect VoiceA.
// ============================================================
class VoiceDopp : public BaseVoice
{
public:
    VoiceDopp() = default;
    ~VoiceDopp() override = default;

    // ------------------------------------------------------------
    // prepare: record sample rate, reset envelopes, clear state
    // ------------------------------------------------------------
    void prepare(double sampleRate) override
    {
        sampleRate_ = sampleRate;
        active_     = false;
        level_      = 0.0f;
        midiNote_   = -1;
    }

    // ------------------------------------------------------------
    // noteOn: activate voice, record note/velocity, zero phase
    // ------------------------------------------------------------
    void noteOn(const ParameterSnapshot& snapshot,
                int midiNote,
                float velocity) override
    {
        juce::ignoreUnused(snapshot, velocity);

        midiNote_ = midiNote;
        active_   = true;
        level_    = 1.0f;     // simple indicator for tests

        // Future: initialize lattice, heading, listener pos, etc.
        phase_ = 0.0;
    }

    // ------------------------------------------------------------
    // noteOff: mark inactive (release handled later in full impl)
    // ------------------------------------------------------------
    void noteOff() override
    {
        active_ = false;
        level_  = 0.0f;
    }

    // ------------------------------------------------------------
    // render: silent output for now (guaranteed safe)
    // ------------------------------------------------------------
    void render(float* buffer, int numSamples) override
    {
        if (!active_)
        {
            std::fill(buffer, buffer + numSamples, 0.0f);
            return;
        }

        // Phase III-A skeleton: silence
        std::fill(buffer, buffer + numSamples, 0.0f);

        // Future placeholder: we can update phase_ to avoid “frozen voice”
        phase_ += (440.0 / sampleRate_) * (double)numSamples;
    }

    // ------------------------------------------------------------
    // state queries
    // ------------------------------------------------------------
    bool isActive() const override     { return active_; }
    int  getNote() const noexcept override { return midiNote_; }
    float getCurrentLevel() const override { return level_; }

    // ------------------------------------------------------------
    // handleController: inert until Doppler CC map added
    // ------------------------------------------------------------
    void handleController(int cc, float norm) override
    {
        juce::ignoreUnused(cc, norm);
        // Inert for safety
    }

private:
    double sampleRate_ = 48000.0;
    bool active_       = false;
    int midiNote_      = -1;
    float level_       = 0.0f;

    // Future field: retarded-time phase accumulator
    double phase_ = 0.0;
};
