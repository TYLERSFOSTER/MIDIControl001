#pragma once
#include "params/ParameterSnapshot.h"

class BaseVoice {
public:
    virtual ~BaseVoice() = default;

    virtual void prepare(double sampleRate) = 0;
    virtual void noteOn(const ParameterSnapshot& snapshot, int midiNote, float velocity) = 0;
    virtual void noteOff() = 0;
    virtual void render(float* buffer, int numSamples) = 0;
    virtual bool isActive() const = 0;

    virtual float getCurrentLevel() const = 0;
    virtual int   getNote() const noexcept = 0;
};
