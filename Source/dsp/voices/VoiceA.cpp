#include "VoiceA.h"
#include <cmath>
#include <fstream>

// ============================================================
// VoiceA: MIDI-note baseline pitch + persistent CC detune
// ============================================================

void VoiceA::prepare(double sampleRate)
{
    osc_.prepare(sampleRate);
    env_.prepare(sampleRate);
}

void VoiceA::noteOn(const ParameterSnapshot& snapshot, int midiNote, float /*velocity*/)
{
    // --- Baseline pitch from MIDI note (A4=69 -> 440 Hz)
    const float baseHz = midiNoteToHz(midiNote);

    // --- Apply persistent detune from CC5 (in semitones)
    const float freqHz = applyDetuneSemis(baseHz, detuneSemis_);

    DBG("VoiceA::noteOn midiNote=" << midiNote
        << " baseHz=" << baseHz
        << " detuneSemis=" << detuneSemis_
        << " => freqHz=" << freqHz);

    osc_.setFrequency(freqHz);
    env_.setAttack(snapshot.envAttack);
    env_.setRelease(snapshot.envRelease);
    osc_.resetPhase();
    env_.noteOn();

    active_ = true;
    note_   = midiNote;
    level_  = 0.0f;
}

void VoiceA::noteOff()
{
    env_.noteOff();
}

bool VoiceA::isActive() const { return active_; }
int  VoiceA::getNote() const noexcept { return note_; }

void VoiceA::render(float* buffer, int numSamples)
{
    if (!active_)
        return;

    float blockPeak = 0.0f;
    float blockSumSq = 0.0f;

    const float envStart = env_.getCurrentValue();
    float envEnd   = envStart;

    const float  freqAtBlock = osc_.getFrequency();
    const double atkInc      = env_.getAttackInc();
    const double relCoef     = env_.getReleaseCoef();
    const double relSec      = env_.getReleaseSec();

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

    // voice auto-deactivate on tail end
    if (!env_.isActive() || blockPeak < 1e-3f)
    {
        active_ = false;
        osc_.setFrequency(0.0f);
        osc_.resetPhase();
    }

    const float rms = std::sqrt(blockSumSq / std::max(1, numSamples));

    DBG("[VoiceA@render] note=" << note_
        << " freqHz=" << freqAtBlock
        << " env(start→end)=" << envStart << "→" << envEnd
        << " atkInc=" << atkInc
        << " relSec=" << relSec
        << " blockRMS=" << rms
        << " peak=" << blockPeak
        << " active=" << (active_ ? "Y" : "N"));

    // optional file log
    std::ofstream log("voice_debug.txt", std::ios::app);
    log << "[VoiceA@render] note=" << note_
        << " freqHz=" << freqAtBlock
        << " env(start→end)=" << envStart << "→" << envEnd
        << " atkInc=" << atkInc
        << " relSec=" << relSec
        << " blockRMS=" << rms
        << " peak=" << blockPeak
        << " active=" << (active_ ? "Y" : "N")
        << "\n";
}

void VoiceA::updateParams(const VoiceParams& vp)
{
    // ⚠️ Do NOT set frequency here — avoids snapping to a global osc value.
    // Keep frequency governed by (MIDI note ⨉ detune), set at noteOn and by CC5 live updates.

    env_.setAttack(vp.envAttack);
    env_.setRelease(vp.envRelease);
}

float VoiceA::getCurrentLevel() const { return level_; }

void VoiceA::handleController(int cc, float norm)
{
    // Throttle duplicate spam
    static float lastAttack = -1.0f;
    static float lastRelease = -1.0f;
    static float lastHz = -1.0f;
    constexpr float epsA = 0.005f;
    constexpr float epsR = 0.05f;
    constexpr float epsF = 2.0f; // report only if live-freq moves ~>2 Hz

    switch (cc)
    {
        case 3: // Attack (perceptual 1 ms → 2 s)
        {
            const float attack = 0.001f * std::pow(2000.0f, norm);
            env_.setAttack(attack);
            if (std::fabs(attack - lastAttack) > epsA) {
                DBG("[CC3] attack=" << attack);
                lastAttack = attack;
            }
            break;
        }
        case 4: // Release (perceptual 20 ms → 5 s)
        {
            const float release = 0.020f * std::pow(250.0f, norm);
            env_.setRelease(release);
            if (std::fabs(release - lastRelease) > epsR) {
                DBG("[CC4] release=" << release);
                lastRelease = release;
            }
            break;
        }
        case 5: // Pitch detune in semitones (±12 semis example)
        {
            detuneSemis_ = -12.0f + 24.0f * norm;

            // If currently active, update oscillator live (recompute from current note)
            if (active_ && note_ >= 0) {
                const float hz = applyDetuneSemis(currentNoteBaseHz(), detuneSemis_);
                osc_.setFrequency(hz);
                if (std::fabs(hz - lastHz) > epsF) {
                    DBG("[CC5] detuneSemis=" << detuneSemis_ << " => oscFreq=" << hz);
                    lastHz = hz;
                }
            } else {
                DBG("[CC5] detuneSemis=" << detuneSemis_);
            }
            break;
        }
        default:
            break;
    }
}
