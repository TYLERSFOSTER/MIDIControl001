// VoiceA.h
#pragma once
#include <juce_core/juce_core.h>
#include "dsp/BaseVoice.h"
#include "dsp/oscillators/OscillatorA.h"
#include "dsp/envelopes/EnvelopeA.h"
#include "params/ParameterSnapshot.h"

// ============================================================
// VoiceA declaration only — implementations live in VoiceA.cpp
// ============================================================

class VoiceA : public BaseVoice {
public:
    void prepare(double sampleRate) override;
    void noteOn(const ParameterSnapshot& snapshot, int midiNote, float velocity) override;
    void noteOff() override;
    bool isActive() const override;
    int  getNote() const noexcept override;
    void render(float* buffer, int numSamples) override;
    float getCurrentLevel() const override;

    // Live parameter modulation (added in Step 7)
    void updateParams(const VoiceParams& vp);

    // ============================================================
    // Phase 4-F-02: per-voice controller mapping (CC3–CC5)
    // Called from VoiceManager::handleController()
    // ============================================================
    void handleController(int cc, float norm) override;

private:
    OscillatorA osc_;
    EnvelopeA   env_;
    bool  active_ = false;
    int   note_   = -1;
    float level_  = 0.0f;
};
