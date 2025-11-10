#include "VoiceA.h"
#include <cmath>
#include <fstream>

// ============================================================
// Step 7 + Phase 4-F-02: VoiceA implementation with diagnostics
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

bool VoiceA::isActive() const { return active_; }

int VoiceA::getNote() const noexcept { return note_; }

void VoiceA::render(float* buffer, int numSamples)
{
    if (!active_)
        return;

    float blockPeak = 0.0f;
    float blockSumSq = 0.0f;

    float envStart = env_.getCurrentValue();
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
        DBG("VoiceA::updateParams skipped freq for active note=" << note_);
    else
        osc_.setFrequency(vp.oscFreq);

    // Envelope parameters remain safely modulatable in real time
    env_.setAttack(vp.envAttack);
    env_.setRelease(vp.envRelease);
}

float VoiceA::getCurrentLevel() const { return level_; }

// ============================================================
// Phase 4-F-02: Per-voice controller mapping (CC3–CC5)
// ============================================================
void VoiceA::handleController(int cc, float norm)
{
    switch (cc)
    {
        case 3: // Attack
        {
            float attack = 0.001f + 1.999f * norm; // 1 ms–2 s
            env_.setAttack(attack);
            DBG("[CC3] attack=" << attack);
            break;
        }

        case 4: // Release
        {
            float release = 0.01f + 4.99f * norm; // 10 ms–5 s
            env_.setRelease(release);
            DBG("[CC4] release=" << release);
            break;
        }

        case 5: // Oscillator frequency sweep
        {
            float base = 440.0f;   // A4
            float range = 2000.0f; // ±2 kHz sweep
            float hz = base + range * (norm - 0.5f);
            osc_.setFrequency(hz);
            DBG("[CC5] oscFreq=" << hz);
            break;
        }

        default:
            break;
    }
}
