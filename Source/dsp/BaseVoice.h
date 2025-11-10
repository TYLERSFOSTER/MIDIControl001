#pragma once
#include "params/ParameterSnapshot.h"

// Base class for all voice implementations (e.g. VoiceLegacy, VoiceA, etc.)
class BaseVoice {
public:
    virtual ~BaseVoice() = default;

    // Core lifecycle
    virtual void prepare(double sampleRate) = 0;
    virtual void noteOn(const ParameterSnapshot& snapshot, int midiNote, float velocity) = 0;
    virtual void noteOff() = 0;

    // Audio render
    virtual void render(float* buffer, int numSamples) = 0;

    // State queries
    virtual bool  isActive() const = 0;
    virtual int   getNote() const noexcept = 0;
    virtual float getCurrentLevel() const = 0;

    // ============================================================
    // Diagnostic stub for per-voice MIDI CC handling
    // ============================================================
    virtual void handleController(int cc, float norm)
    {
        (void)cc;
        (void)norm;
    }
};
