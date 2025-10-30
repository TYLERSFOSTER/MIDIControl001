#pragma once
#include <juce_core/juce_core.h>
#include "dsp/oscillators/OscillatorA.h"
#include "dsp/envelopes/EnvelopeA.h"
#include "params/ParameterSnapshot.h"
#include <cmath>

class Voice {
public:
    void prepare(double sampleRate)
    {
        osc_.prepare(sampleRate);
        env_.prepare(sampleRate);
    }

    void noteOn(const ParameterSnapshot& snapshot, int midiNote, float velocity)
    {
        const float freqHz = snapshot.oscFreq > 0.f
            ? snapshot.oscFreq
            : 440.0f * std::pow(2.0f, (midiNote - 69) / 12.0f);

        osc_.setFrequency(freqHz);
        env_.setAttack(snapshot.envAttack);
        env_.setRelease(snapshot.envRelease);
        osc_.resetPhase();
        env_.noteOn();

        active_ = true;
        note_ = midiNote;
        level_ = 0.0f;
    }

    void noteOff() { env_.noteOff(); }

    bool isActive() const { return active_; }

    int getNote() const noexcept { return note_; }

    void render(float* buffer, int numSamples)
    {
        if (!active_)
            return;

        DBG("Voice[" << note_ << "] render start, level=" << level_);

        float blockPeak = 0.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            const float envValue = env_.nextSample();
            const float sample   = osc_.nextSample() * envValue;
            buffer[i] += sample;
            blockPeak = std::max(blockPeak, std::fabs(sample));
        }

        level_ = blockPeak;

        if (!env_.isActive() || blockPeak < 1e-3f)
        {
            DBG("Voice[" << note_ << "] deactivating, peak=" << blockPeak);
            active_ = false;
            osc_.setFrequency(0.0f);
            osc_.resetPhase();
            return;
        }

        DBG("Voice[" << note_ << "] still active, peak=" << blockPeak);
    }

    float getCurrentLevel() const { return level_; }

private:
    OscillatorA osc_;
    EnvelopeA   env_;
    bool  active_ = false;
    int   note_   = -1;
    float level_  = 0.0f;
};
