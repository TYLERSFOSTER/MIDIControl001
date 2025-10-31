#pragma once
#include <juce_core/juce_core.h>
#include "dsp/BaseVoice.h"
#include "dsp/oscillators/OscillatorA.h"
#include "dsp/envelopes/EnvelopeA.h"
#include "params/ParameterSnapshot.h"
#include <cmath>

class VoiceA : public BaseVoice {
public:
    void prepare(double sampleRate) override
    {
        osc_.prepare(sampleRate);
        env_.prepare(sampleRate);
    }

    void noteOn(const ParameterSnapshot& snapshot, int midiNote, float velocity) override
    {
        const float defaultFreq = 440.0f;
        const float midiFreq = defaultFreq * std::pow(2.0f, (midiNote - 69) / 12.0f);

        // use snapshot only if user actually moved that parameter
        const float freqHz = std::fabs(snapshot.oscFreq - defaultFreq) > 1e-3f
            ? snapshot.oscFreq
            : midiFreq;

        DBG("VoiceA::noteOn -> midiNote=" << midiNote
            << " snapshot.oscFreq=" << snapshot.oscFreq
            << "  => freqHz=" << freqHz);

        osc_.setFrequency(freqHz);
        env_.setAttack(snapshot.envAttack);
        env_.setRelease(snapshot.envRelease);
        osc_.resetPhase();
        env_.noteOn();

        active_ = true;
        note_ = midiNote;
        level_ = 0.0f;
    }

    void noteOff() override { env_.noteOff(); }
    bool isActive() const override { return active_; }
    int getNote() const noexcept override { return note_; }

    void render(float* buffer, int numSamples) override
    {
        if (!active_)
            return;

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
            active_ = false;
            osc_.setFrequency(0.0f);
            osc_.resetPhase();
            return;
        }
    }

    float getCurrentLevel() const override { return level_; }

private:
    OscillatorA osc_;
    EnvelopeA   env_;
    bool  active_ = false;
    int   note_   = -1;
    float level_  = 0.0f;
};
