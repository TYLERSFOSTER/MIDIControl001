#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include "dsp/BaseVoice.h"
#include "dsp/voices/VoiceA.h"

// -----------------------------------------------------------------------------
// Simple registry: name → factory function returning a BaseVoice
// -----------------------------------------------------------------------------
class InstrumentRegistry {
public:
    using FactoryFn = std::unique_ptr<BaseVoice>(*)();

    static InstrumentRegistry& instance()
    {
        static InstrumentRegistry registry;
        return registry;
    }

    void registerVoice(const std::string& name, FactoryFn fn)
    {
        factories_[name] = fn;
    }

    std::unique_ptr<BaseVoice> makeVoice(const std::string& name) const
    {
        if (auto it = factories_.find(name); it != factories_.end())
            return (it->second)();
        return nullptr;
    }

private:
    std::unordered_map<std::string, FactoryFn> factories_;

    InstrumentRegistry()
    {
        registerVoice("voiceA", &InstrumentRegistry::makeVoiceA);
    }

    // Static wrapper for function-pointer compatibility
    static std::unique_ptr<BaseVoice> makeVoiceA()
    {
        return std::make_unique<VoiceA>();
    }
};
