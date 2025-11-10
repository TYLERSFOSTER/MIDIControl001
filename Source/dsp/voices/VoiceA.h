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

    // Per-voice controller mapping (CC3–CC5)
    void handleController(int cc, float norm) override;

    // Persistent detune API (VoiceManager sets this on CC5)
    void setDetuneSemis(float s) noexcept { detuneSemis_ = s; }
    float getDetuneSemis() const noexcept { return detuneSemis_; }

private:
    static inline float midiNoteToHz(int note) noexcept {
        return 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
    }
    static inline float applyDetuneSemis(float hz, float semis) noexcept {
        return hz * std::pow(2.0f, semis / 12.0f);
    }

    float currentNoteBaseHz() const noexcept {
        return (note_ >= 0) ? midiNoteToHz(note_) : 440.0f;
    }

    OscillatorA osc_;
    EnvelopeA   env_;
    bool  active_ = false;
    int   note_   = -1;
    float level_  = 0.0f;

    // ✅ persistent semitone detune applied at noteOn and during live CC5 moves
    float detuneSemis_ = 0.0f;
};
