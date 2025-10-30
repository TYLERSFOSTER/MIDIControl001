#pragma once
#include <juce_core/juce_core.h>
#include <vector>
#include <memory>
#include <algorithm>
#include "dsp/Voice.h"

class VoiceManager {
public:
    static constexpr int maxVoices = 32;

    void prepare(double sampleRate) {
        voices_.clear();
        voices_.reserve(maxVoices);
        for (int i = 0; i < maxVoices; ++i) {
            auto v = std::make_unique<Voice>();
            v->prepare(sampleRate);
            voices_.push_back(std::move(v));
        }

        DBG("VoiceManager prepared " + juce::String(voices_.size()) +
            " voices at " + juce::String(sampleRate));
    }

    void startBlock(const ParameterSnapshot& snapshot) {
        currentSnapshot_ = &snapshot;
    }

    void handleNoteOn(int midiNote, float velocity) {
        if (!currentSnapshot_) return;

        // find free voice
        auto it = std::find_if(voices_.begin(), voices_.end(),
                               [](const auto& v) { return !v->isActive(); });

        if (it == voices_.end()) {
            // all active → steal quietest
            it = std::min_element(voices_.begin(), voices_.end(),
                                  [](const auto& a, const auto& b) {
                                      return a->getCurrentLevel() < b->getCurrentLevel();
                                  });
        }

        (*it)->noteOn(*currentSnapshot_, midiNote, velocity);
    }

    void handleNoteOff(int midiNote) {
        for (auto& v : voices_) {
            if (v->isActive() && v->getNote() == midiNote)
                v->noteOff();
        }
    }

    void render(float* buffer, int numSamples)
    {
        // Clear once at start of block
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

        DBG("VoiceManager: active voices after render = " << activeCount);
    }

private:
    std::vector<std::unique_ptr<Voice>> voices_;
    const ParameterSnapshot* currentSnapshot_ = nullptr;
};
