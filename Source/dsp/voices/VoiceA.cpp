#include "VoiceA.h"
#include <cmath>

// ============================================================
// Step 7: VoiceA implementation with live parameter updates
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
    }
}

// ============================================================
// Step 7 addition: real-time parameter updates
// ============================================================
void VoiceA::updateParams(const VoiceParams& vp)
{
    if (!active_)
        return;

    osc_.setFrequency(vp.oscFreq);
    env_.setAttack(vp.envAttack);
    env_.setRelease(vp.envRelease);

    DBG("VoiceA::updateParams() applied live: "
        << "freq=" << vp.oscFreq
        << " atk=" << vp.envAttack
        << " rel=" << vp.envRelease);
}

float VoiceA::getCurrentLevel() const
{
    return level_;
}
