#pragma once
#include <juce_core/juce_core.h>
#include <vector>
#include <memory>
#include <algorithm>
#include <fstream>
#include <numeric>   // for inner_product
#include <cmath>     // for std::exp, std::log
#include <functional>

#include "params/ParameterSnapshot.h"
#include "dsp/voices/VoiceA.h"
#include "dsp/voices/VoiceDopp.h"
#include "dsp/BaseVoice.h"
#include "params/ParamLayout.h"

// ============================================================
// VoiceManager — manages voice allocation and clickless summation
// ============================================================

class VoiceManager {
public:
    using SnapshotMaker = std::function<ParameterSnapshot(void)>; // callback type

    // Optional injection point for mode-aware voice construction.
    // If empty, we fall back to makeVoiceForMode(mode_).
    using VoiceFactory  = std::function<std::unique_ptr<BaseVoice>(VoiceMode)>;

    explicit VoiceManager(SnapshotMaker makeSnapshot,
                          VoiceFactory voiceFactory = {})
        : makeSnapshot_(std::move(makeSnapshot)),
          voiceFactory_(std::move(voiceFactory))
    {}

    static constexpr int maxVoices = 32;

    // ============================================================
    // Phase III — Mode-aware voice factory (currently inert)
    // ============================================================
    std::unique_ptr<BaseVoice> makeVoiceForMode(VoiceMode mode) const
    {
        // Phase III scaffolding:
        // All modes currently instantiate VoiceA to keep DSP identical.
        switch (mode)
        {
            case VoiceMode::VoiceA:
                return std::make_unique<VoiceA>();

            case VoiceMode::VoiceDopp:
                return std::make_unique<VoiceDopp>();

            case VoiceMode::VoiceLET:
            case VoiceMode::VoiceFM:
            default:
                return std::make_unique<VoiceA>();
        }
    }

    // Central hook for future mode-specific per-block behavior.
    void applyModeConfiguration()
    {
        switch (mode_)
        {
            case VoiceMode::VoiceA:
            case VoiceMode::VoiceDopp:
            case VoiceMode::VoiceLET:
            case VoiceMode::VoiceFM:
            default:
                // Current plugin: only VoiceA exists, nothing to do yet.
                break;
        }
    }

    // ============================================================
    // Phase III B6 — mode-aware routing entry point
    // Still inert, but now explicitly logs and forms the dispatch hub
    // ============================================================
    void applyModeRouting(const ParameterSnapshot& snapshot)
    {
        juce::ignoreUnused(snapshot);

        DBG("VoiceManager::applyModeRouting(mode=" 
            << static_cast<int>(mode_) << ")");

        switch (mode_)
        {
            case VoiceMode::VoiceA:
                // Standard VoiceA → normal per-sample render path
                break;

            case VoiceMode::VoiceDopp:
                // Future: Doppler block graph routing
                // For now: identical behavior to VoiceA
                break;

            case VoiceMode::VoiceLET:
                // Future: LET time-warping graph
                // For now: identical behavior to VoiceA
                break;

            case VoiceMode::VoiceFM:
                // Future: FM operator graph dispatch
                // For now: identical behavior to VoiceA
                break;

            default:
                break;
        }
    }

    // ============================================================
    // Phase II / III — Global voice-mode state (A→D)
    // ============================================================
    void setMode(VoiceMode m)
    {
        mode_ = m;
    }

    VoiceMode getMode() const noexcept
    {
        return mode_;
    }

    // ============================================================
    // Phase III – B4: Injectable Voice Factory Setter
    // Allows external code (processor/tests) to override voice construction.
    // ============================================================
    void setVoiceFactory(VoiceFactory factory)
    {
        voiceFactory_ = std::move(factory);
        rebuildVoicesForMode();
    }

    void prepare(double sampleRate)
    {
        sampleRate_ = sampleRate;
        globalGain_.reset(sampleRate, 0.005); // 5 ms fade on poly changes

        // Phase III B7 — ensure lastMode_ is in sync at startup.
        lastMode_ = mode_;

        // Phase III B6 — centralize voice allocation in a helper.
        rebuildVoicesForMode();

        DBG("VoiceManager prepared " + juce::String(voices_.size()) +
            " voices at " + juce::String(sampleRate));
    }

    void startBlock()
    {
        static ParameterSnapshot snapshot;
        snapshot = makeSnapshot_();

        // ============================================================
        // Phase III B9 — mode-aware routing hook (NEW, currently inert)
        // ============================================================
        applyModeRouting(snapshot);

        // ============================================================
        // Phase III B8 — mode-change detection (NEW)
        // ============================================================
        rebuildVoicesIfModeChanged();

        // Phase III B2 — mode hook (currently inert)
        applyModeConfiguration();

        // ============================================================
        // Phase 5-C.4 — Persistent CC Cache Re-application
        // ============================================================
        snapshot.envAttack  = ccCache.envAttack;
        snapshot.envRelease = ccCache.envRelease;
        snapshot.oscFreq    = ccCache.oscFreq;

        currentSnapshot_ = &snapshot;

        // ============================================================
        // Phase III – B3: reconcile global vs per-voice params
        //
        // Effective per-voice parameters are derived from snapshot.voices[i],
        // but the *global* (possibly CC-modified) env/freq fields win.
        //
        // This keeps APVTS + snapshot semantics intact for tests, while
        // ensuring that the actual DSP hears the CC-driven envelope/freq.
        // ============================================================
        for (int i = 0; i < static_cast<int>(voices_.size()) && i < NUM_VOICES; ++i)
        {
            // Start from whatever per-voice parameters the snapshot captured
            VoiceParams vp = snapshot.voices[i];

            // Override with global, CC-modified values
            vp.oscFreq    = snapshot.oscFreq;
            vp.envAttack  = snapshot.envAttack;
            vp.envRelease = snapshot.envRelease;

            if (auto* voiceA = dynamic_cast<VoiceA*>(voices_[i].get()))
                voiceA->updateParams(vp);
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
    // ============================================================
    // Phase III B6 — central voice rebuild helper
    // ============================================================
    void rebuildVoicesForMode()
    {
        voices_.clear();
        voices_.reserve(maxVoices);

        // Same factory semantics as before: injectable → fallback.
        auto makeVoice = [this](VoiceMode m)
        {
            if (voiceFactory_)
                return voiceFactory_(m);
            return makeVoiceForMode(m);
        };

        for (int i = 0; i < maxVoices; ++i)
        {
            auto v = makeVoice(mode_);
            v->prepare(sampleRate_);
            voices_.push_back(std::move(v));
        }
    }

    // ============================================================
    // Phase III – B7: Robust mode-change detection & guardrails
    // ============================================================
    void rebuildVoicesIfModeChanged()
    {
        // No change → nothing to do
        if (mode_ == lastMode_)
            return;

        // Guardrail: detect mid-block thrash (should never happen)
        DBG("VoiceManager: MODE CHANGE detected "
            << static_cast<int>(lastMode_) << " → "
            << static_cast<int>(mode_));

        // Commit the mode change
        lastMode_ = mode_;

        // Rebuild all voices according to the new mode
        rebuildVoicesForMode();
    }

    std::vector<std::unique_ptr<BaseVoice>> voices_;
    const ParameterSnapshot* currentSnapshot_ = nullptr;
    SnapshotMaker makeSnapshot_;  // stored callback

    // Optional, mode-aware voice factory (B4). If empty, we defer to makeVoiceForMode().
    VoiceFactory voiceFactory_;

    // ============================================================
    // Phase II / III — Global voice-mode state (A→D)
    // ============================================================
    VoiceMode mode_     = VoiceMode::VoiceA;  // default
    VoiceMode lastMode_ = VoiceMode::VoiceA;  // cached for future B8 wiring

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
