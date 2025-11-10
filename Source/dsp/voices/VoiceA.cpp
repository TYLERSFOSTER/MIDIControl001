// VoiceA.cpp
#include "VoiceA.h"
#include <cmath>
#include <fstream>

// ============================================================
// Step 7: VoiceA implementation with diagnostics
// ============================================================

void VoiceA::prepare(double sampleRate)
{
    osc_.prepare(sampleRate);
    env_.prepare(sampleRate);
}

void VoiceA::noteOn(const ParameterSnapshot& snapshot, int midiNote, float velocity)
{
    const float defaultFreq = 440.0f;
    const float midiFreq = defaultFreq * std::pow(2.0f, (midiNote - 69) / 12.0f);

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

void VoiceA::noteOff()
{
    env_.noteOff();
}

bool VoiceA::isActive() const
{
    return active_;
}

int VoiceA::getNote() const noexcept
{
    return note_;
}

void VoiceA::render(float* buffer, int numSamples)
{
    if (!active_)
        return;

    float blockPeak = 0.0f;
    float blockSumSq = 0.0f;

    float envStart = env_.getCurrentValue(); // assume EnvelopeA exposes this
    float envEnd   = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        const float envValue = env_.nextSample();
        const float oscValue = osc_.nextSample();
        const float sample   = oscValue * envValue;

        buffer[i] += sample;
        blockPeak = std::max(blockPeak, std::fabs(sample));
        blockSumSq += sample * sample;

        if (i == numSamples - 1)
            envEnd = envValue;
    }

    level_ = blockPeak;

    if (!env_.isActive() || blockPeak < 1e-3f)
    {
        active_ = false;
        osc_.setFrequency(0.0f);
        osc_.resetPhase();
    }

    const float rms = std::sqrt(blockSumSq / numSamples);

    {
        std::ofstream log("voice_debug.txt", std::ios::app);
        log << "[VoiceA] note=" << note_
            << " env(start→end)=" << envStart << "→" << envEnd
            << " blockRMS=" << rms
            << " peak=" << blockPeak
            << " active=" << (active_ ? "Y" : "N")
            << "\n";
    }

    DBG("[VoiceA] note=" << note_
        << " env(start→end)=" << envStart << "→" << envEnd
        << " blockRMS=" << rms
        << " peak=" << blockPeak
        << " active=" << (active_ ? "Y" : "N"));
}

void VoiceA::updateParams(const VoiceParams& vp)
{
    // ============================================================
    // FIX: prevent overwriting per-voice pitch during active notes
    // ============================================================
    if (active_)
    {
        DBG("VoiceA::updateParams skipped freq for active note=" << note_);
    }
    else
    {
        osc_.setFrequency(vp.oscFreq);
    }

    // Envelope parameters remain safely modulatable in real time
    env_.setAttack(vp.envAttack);
    env_.setRelease(vp.envRelease);
}

float VoiceA::getCurrentLevel() const
{
    return level_;
}
