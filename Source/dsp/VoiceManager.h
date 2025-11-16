#pragma once
#include <juce_core/juce_core.h>
#include <vector>
#include <memory>
#include <algorithm>
#include <fstream>
#include <numeric> // for inner_product
#include "params/ParameterSnapshot.h"
#include "dsp/voices/VoiceA.h"
#include "dsp/BaseVoice.h"
#include "params/ParamLayout.h"

// ============================================================
// VoiceManager — manages voice allocation and clickless summation
// ============================================================

class VoiceManager {
public:
    using SnapshotMaker = std::function<ParameterSnapshot(void)>; // callback type

    explicit VoiceManager(SnapshotMaker makeSnapshot)
        : makeSnapshot_(std::move(makeSnapshot)) {}

    static constexpr int maxVoices = 32;

    // ============================================================
    // Phase II — Global voice-mode state (A→D)
    // ============================================================
    void setMode(int m)
    {
        mode_ = m;   // 0 == "voiceA" (current-only)
    }

    // >>> ADDED FOR A6 <<<
    int getMode() const noexcept
    {
        return mode_;
    }
    // <<< END ADDED >>>

    void prepare(double sampleRate)
    {
        sampleRate_ = sampleRate;
        globalGain_.reset(sampleRate, 0.005); // 5 ms fade on poly changes

        voices_.clear();
        voices_.reserve(maxVoices);
        for (int i = 0; i < maxVoices; ++i)
        {
            auto v = std::make_unique<VoiceA>();
            v->prepare(sampleRate);
            voices_.push_back(std::move(v));
        }

        DBG("VoiceManager prepared " + juce::String(voices_.size()) +
            " voices at " + juce::String(sampleRate));
    }

    void startBlock()
    {
        static ParameterSnapshot snapshot;
        snapshot = makeSnapshot_();

        // ============================================================
        // Phase 5-C.4 — Persistent CC Cache Re-application
        // ============================================================
        snapshot.envAttack  = ccCache.envAttack;
        snapshot.envRelease = ccCache.envRelease;
        snapshot.oscFreq    = ccCache.oscFreq;

        currentSnapshot_ = &snapshot;

        for (int i = 0; i < static_cast<int>(voices_.size()) && i < NUM_VOICES; ++i)
        {
            if (auto* voiceA = dynamic_cast<VoiceA*>(voices_[i].get()))
                voiceA->updateParams(snapshot.voices[i]);
        }
    }

    void handleNoteOn(int midiNote, float velocity)
    {
        if (!currentSnapshot_) return;

        auto it = std::find_if(voices_.begin(), voices_.end(),
                               [](const auto& v) { return !v->isActive(); });

        if (it == voices_.end())
        {
            it = std::min_element(voices_.begin(), voices_.end(),
                                  [](const auto& a, const auto& b) {
                                      return a->getCurrentLevel() < b->getCurrentLevel();
                                  });
        }

        (*it)->noteOn(*currentSnapshot_, midiNote, velocity);

        DBG("[VM] NoteOn midiNote=" << midiNote);
        std::cout << "[VM] NoteOn midiNote=" << midiNote << std::endl;

        globalGain_.setTargetValue(1.0f);
    }

    void handleNoteOff(int midiNote)
    {
        for (auto& v : voices_)
        {
            if (v->isActive() && v->getNote() == midiNote)
                v->noteOff();
        }

        if (std::none_of(voices_.begin(), voices_.end(),
                         [](const auto& v){ return v->isActive(); }))
        {
            globalGain_.setTargetValue(0.0f);
        }
    }

    // ============================================================
    // Phase 5-C.4 — Persistent CC Cache + Dispatch
    // ============================================================
    void handleController(int cc, float norm)
    {
        std::ofstream log("voice_debug.txt", std::ios::app);
        if (log.is_open())
            log << "dispatch cc=" << cc << " norm=" << norm << std::endl;

        std::cout << "[VoiceManager] dispatch cc=" << cc << " norm=" << norm << std::endl;
        DBG("dispatch cc=" << cc << " norm=" << norm);

        // Cache CC values for future snapshots
        switch (cc)
        {
            case 3: // Attack: 1 ms → 2 s exponential
                ccCache.envAttack = std::exp(juce::jmap(norm, std::log(0.001f), std::log(2.0f)));
                break;

            case 4: // Release: 20 ms → 5 s logarithmic
                ccCache.envRelease = std::exp(juce::jmap(norm, std::log(0.02f), std::log(5.0f)));
                break;

            case 5: // Pitch sweep unchanged
                ccCache.oscFreq = 440.0f + 2000.0f * (norm - 0.5f);
                break;

            default: break;
        }

        for (auto& v : voices_)
            v->handleController(cc, norm);
    }

    void render(float* buffer, int numSamples)
    {
        std::fill(buffer, buffer + numSamples, 0.0f);
        int activeCount = 0;

        for (auto& v : voices_)
        {
            if (v->isActive())
            {
                ++activeCount;
                v->render(buffer, numSamples);
            }
        }

        float blockSumSq = std::inner_product(buffer, buffer + numSamples, buffer, 0.0f);
        float preGainRMS = std::sqrt(blockSumSq / numSamples);

        const float targetRMS   = 0.26f;
        const float eps         = 1e-6f;
        const float measured    = std::max(preGainRMS, eps);
        const float ctrl        = juce::jlimit(0.25f, 4.0f, targetRMS / measured);

        const float prevTarget  = globalGain_.getTargetValue();
        const float blended     = 0.9f * prevTarget + 0.1f * ctrl;
        globalGain_.setTargetValue(juce::jlimit(0.25f, 4.0f, blended));

        std::ofstream log("voice_debug.txt", std::ios::app);
        if (!log.is_open())
        {
            DBG("VoiceManager: FAILED to open voice_debug.txt");
            return;
        }

        log << "[VoiceManager] pre-gain RMS=" << preGainRMS
            << " start=" << globalGain_.getCurrentValue();

        std::cout << "[DIAG] globalGain start=" << globalGain_.getCurrentValue()
                  << " end=" << globalGain_.getTargetValue() << std::endl;

        for (int i = 0; i < numSamples; ++i)
        {
            float g = globalGain_.getNextValue();
            buffer[i] *= g;
        }

        float postGainRMS = std::sqrt(std::inner_product(buffer, buffer + numSamples, buffer, 0.0f) / numSamples);
        DBG("VoiceManager: postGainRMS = " << postGainRMS);

        log << " end=" << globalGain_.getCurrentValue()
            << " active=" << activeCount << std::endl;
    }

private:
    std::vector<std::unique_ptr<BaseVoice>> voices_;
    const ParameterSnapshot* currentSnapshot_ = nullptr;
    SnapshotMaker makeSnapshot_;  // stored callback

    // ============================================================
    // Phase II — Global voice-mode state (A→D)
    // ============================================================
    int mode_ = 0;  // 0 == "voiceA" (only supported mode right now)

    // ============================================================
    // Persistent CC cache (Phase 5-C.4)
    // ============================================================
    struct {
        float envAttack  = 0.01f;
        float envRelease = 0.20f;
        float oscFreq    = 440.0f;
    } ccCache;

    double sampleRate_ = 48000.0;
    juce::SmoothedValue<float> globalGain_{ 1.0f }; // clickless poly gain
};
