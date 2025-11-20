#pragma once

#include "dsp/BaseVoice.h"
#include "params/ParameterSnapshot.h"

#include <cmath>
#include <atomic>
#include <limits>  // for std::numeric_limits

// We need juce::Point; pull in the graphics module.
#include <juce_graphics/juce_graphics.h>

// ============================================================
// VoiceDopp — Phase IV Action 1–7
// ------------------------------------------------------------
// Action 1: Kinematic API (position, time, listener controls)
// Action 2: Time accumulator (feature-gated, still OFF)
// Action 3.1: Heading + speed mapping (pure math only)
// Action 4: Listener trajectory integration (gated)
// Action 5: Emitter lattice construction (pure math only)
// Action 6: Distance + retarded time (pure math only)
// Action 7: Source functions at retarded time (pure math only)
// Action 8: Predictive Scoring (public DSP API)
// ============================================================

class VoiceDopp : public BaseVoice
{
public:
    VoiceDopp()  = default;
    ~VoiceDopp() override = default;

    // ------------------------------------------------------------
    // prepare()
    // ------------------------------------------------------------
    void prepare(double sampleRate) override
    {
        sampleRate_ = sampleRate;
        active_     = false;
        midiNote_   = -1;
        level_      = 0.0f;

        // Kinematic state
        listenerPos_ = { 0.0f, 0.0f };
        timeSec_     = 0.0;
        speedNorm_   = 0.0f;
        headingNorm_ = 0.5f;   // θ = 0

        // Emitter field state (Action 5)
        densityNorm_     = 0.0f;  // ρ = 0 -> single line case
        orientationNorm_ = 0.0f;  // φ = -π by default

        // Action-2 gate stays off by default
        enableTimeAccumulation_ = false;

        // Action-7: reset envelope times to defaults
        noteOnTimeSec_  = 0.0;
        noteOffTimeSec_ = std::numeric_limits<double>::infinity();
    }

    // ------------------------------------------------------------
    // noteOn()
    // ------------------------------------------------------------
    void noteOn(const ParameterSnapshot& snapshot,
                int midiNote,
                float velocity) override
    {
        (void)snapshot;
        (void)velocity;

        midiNote_ = midiNote;
        active_   = true;
        level_    = 1.0f;

        listenerPos_ = { 0.0f, 0.0f };
        timeSec_     = 0.0;

        // For now we *do not* couple ADSR timing to noteOn/noteOff.
        // Action 7 keeps source functions purely mathematical.
        noteOnTimeSec_  = 0.0;
        noteOffTimeSec_ = std::numeric_limits<double>::infinity();
    }

    // ------------------------------------------------------------
    // noteOff()
    // ------------------------------------------------------------
    void noteOff() override
    {
        active_ = false;
        level_  = 0.0f;

        // ADSR release coupling to noteOff will be wired later,
        // when we hook real audio rendering. For Action 7 we keep
        // the source math independent.
    }

    // ------------------------------------------------------------
    // render()
    // ------------------------------------------------------------
    void render(float* buffer, int numSamples) override
    {
        if (!active_)
        {
            std::fill(buffer, buffer + numSamples, 0.0f);
            return;
        }

        // ======================================
        // Action-2 + Action-4 (gated)
        // ======================================
        if (enableTimeAccumulation_ && sampleRate_ > 0.0)
        {
            // Total dt for this block
            double dt = static_cast<double>(numSamples) / sampleRate_;

            // Accumulate global listener time
            timeSec_ += dt;

            // ---- Action 4: listener trajectory integration ----
            double speed = computeSpeed();
            auto   uvec  = computeUnitVector(); // float pair

            double vx = speed * static_cast<double>(uvec.x);
            double vy = speed * static_cast<double>(uvec.y);

            listenerPos_.x += static_cast<float>(vx * dt);
            listenerPos_.y += static_cast<float>(vy * dt);
        }

        // Still silent for Actions 1–7
        std::fill(buffer, buffer + numSamples, 0.0f);
    }

    // ------------------------------------------------------------
    // Action-1: Kinematic API
    // ------------------------------------------------------------
    void setListenerControls(float speedNorm, float headingNorm)
    {
        speedNorm_   = speedNorm;
        headingNorm_ = headingNorm;
    }

    juce::Point<float> getListenerPosition() const
    {
        return listenerPos_;
    }

    double getListenerTimeSeconds() const
    {
        return timeSec_;
    }

    // ------------------------------------------------------------
    // Action-3.1: Pure mapping functions (NO behavioural change)
    // ------------------------------------------------------------
    double computeHeadingAngle() const
    {
        // θ = 2π * headingNorm − π
        return (2.0 * M_PI * static_cast<double>(headingNorm_)) - M_PI;
    }

    double computeSpeed() const
    {
        // v = v_max * speedNorm
        return vMax_ * static_cast<double>(speedNorm_);
    }

    juce::Point<float> computeUnitVector() const
    {
        double theta = computeHeadingAngle();
        return {
            static_cast<float>(std::cos(theta)),
            static_cast<float>(std::sin(theta))
        };
    }

    // ------------------------------------------------------------
    // Action-2 control (tests use this explicitly)
    // ------------------------------------------------------------
    void enableTimeAccumulation(bool shouldEnable)
    {
        enableTimeAccumulation_ = shouldEnable;
    }

    // ------------------------------------------------------------
    // Action-5: Emitter lattice (pure math, per-voice field)
    // ------------------------------------------------------------

    // Set emitter field controls (ρ, φ) as normalized [0,1] knobs.
    // ρ = densityNorm in [0,1]; φ = 2π * orientationNorm − π.
    void setEmitterFieldControls(float densityNorm, float orientationNorm)
    {
        densityNorm_     = densityNorm;
        orientationNorm_ = orientationNorm;
    }

    // Orientation angle φ = 2π * orientationNorm − π.
    double computeEmitterOrientationAngle() const
    {
        return (2.0 * M_PI * static_cast<double>(orientationNorm_)) - M_PI;
    }

    // Normal vector n(φ) = (cos φ, sin φ).
    juce::Point<float> computeEmitterNormal() const
    {
        double phi = computeEmitterOrientationAngle();
        return {
            static_cast<float>(std::cos(phi)),
            static_cast<float>(std::sin(phi))
        };
    }

    // Tangent vector b(φ) = (−sin φ, cos φ).
    juce::Point<float> computeEmitterTangent() const
    {
        double phi = computeEmitterOrientationAngle();
        return {
            static_cast<float>(-std::sin(phi)),
            static_cast<float>( std::cos(phi))
        };
    }

    // Density ρ ∈ [0,1] derived from knob.
    double computeDensity() const
    {
        return static_cast<double>(densityNorm_);
    }

    // Perpendicular spacing Δ⊥:
    // ρ = 0   -> +∞ (single line k = 0 in the spec)
    // ρ > 0   -> 1 / ρ
    double computeDeltaPerp() const
    {
        double rho = computeDensity();
        if (rho <= 0.0)
            return std::numeric_limits<double>::infinity();

        return 1.0 / rho;
    }

    // Along-line spacing Δ∥ (fixed to 1.0 unless design changes).
    double computeDeltaParallel() const
    {
        return deltaParallel_;
    }

    // Emitter coordinate:
    // x_{k,m} = k Δ⊥ n(φ) + m Δ∥ b(φ)
    juce::Point<float> computeEmitterPosition(int k, int m) const
    {
        double dPerp = computeDeltaPerp();
        double dPar  = computeDeltaParallel();

        auto n = computeEmitterNormal();
        auto b = computeEmitterTangent();

        double nx = static_cast<double>(n.x);
        double ny = static_cast<double>(n.y);
        double bx = static_cast<double>(b.x);
        double by = static_cast<double>(b.y);

        double x = static_cast<double>(k) * dPerp * nx
                 + static_cast<double>(m) * dPar  * bx;
        double y = static_cast<double>(k) * dPerp * ny
                 + static_cast<double>(m) * dPar  * by;

        return {
            static_cast<float>(x),
            static_cast<float>(y)
        };
    }

    // ------------------------------------------------------------
    // Action-6: Distance + Retarded Time (pure math helpers)
    // ------------------------------------------------------------

    // r_i(t) = || x_i – x_L(t) ||
    double computeDistanceToEmitter(const juce::Point<float>& emitterPos) const
    {
        double dx = static_cast<double>(emitterPos.x) - static_cast<double>(listenerPos_.x);
        double dy = static_cast<double>(emitterPos.y) - static_cast<double>(listenerPos_.y);
        return std::sqrt(dx * dx + dy * dy);
    }

    // t_ret = t - r / c, using current listener timeSec_.
    double computeRetardedTime(double distance) const
    {
        return timeSec_ - distance / speedOfSound_;
    }

    // ------------------------------------------------------------
    // Action-7: Source functions evaluated at retarded time
    // ------------------------------------------------------------
    //
    // These are *pure* functions of t_ret.

    void setAdsrParamsForTest(double attackSec,
                              double decaySec,
                              double sustainLevel,
                              double releaseSec)
    {
        adsrAttackSec_     = attackSec;
        adsrDecaySec_      = decaySec;
        adsrSustainLevel_  = sustainLevel;
        adsrReleaseSec_    = releaseSec;
    }

    void setAdsrTimesForTest(double tOn, double tOff)
    {
        noteOnTimeSec_  = tOn;
        noteOffTimeSec_ = tOff;
    }

    void setBaseFrequencyForTest(double freqHz)
    {
        baseFrequencyHz_ = freqHz;
    }

    void setFieldPulseFrequencyForTest(double freqHz)
    {
        fieldPulseHz_ = freqHz;
    }

    double evalCarrierAtRetardedTime(double tRet) const
    {
        double phase = 2.0 * M_PI * baseFrequencyHz_ * tRet + basePhaseRad_;
        return std::sin(phase);
    }

    double evalFieldPulseAtRetardedTime(double tRet) const
    {
        double phase = 2.0 * M_PI * fieldPulseHz_ * tRet;
        return 0.5 * (1.0 + std::sin(phase));
    }

    double evalAdsrAtRetardedTime(double tRet) const
    {
        double t = tRet - noteOnTimeSec_;

        if (t <= 0.0)
            return 0.0;

        const double attack  = adsrAttackSec_;
        const double decay   = adsrDecaySec_;
        const double release = adsrReleaseSec_;
        const double sustain = adsrSustainLevel_;

        const double attackEnd = attack;
        const double decayEnd  = attack + decay;

        const bool hasRelease  =
            std::isfinite(noteOffTimeSec_) && release > 0.0;

        const double tReleaseStart =
            hasRelease
                ? (noteOffTimeSec_ - noteOnTimeSec_)
                : std::numeric_limits<double>::infinity();

        if (!hasRelease || t <= tReleaseStart)
        {
            if (attack > 0.0 && t < attackEnd)
            {
                return t / attack;
            }

            if (decay > 0.0 && t < decayEnd)
            {
                double u = (t - attackEnd) / decay;
                return 1.0 + (sustain - 1.0) * u;
            }

            return sustain;
        }

        double tRel = t - tReleaseStart;
        if (tRel >= release)
            return 0.0;

        double envAtReleaseStart = 0.0;

        if (tReleaseStart <= 0.0)
        {
            envAtReleaseStart = 0.0;
        }
        else if (attack > 0.0 && tReleaseStart < attackEnd)
        {
            envAtReleaseStart = tReleaseStart / attack;
        }
        else if (decay > 0.0 && tReleaseStart < decayEnd)
        {
            double u = (tReleaseStart - attackEnd) / decay;
            envAtReleaseStart = 1.0 + (sustain - 1.0) * u;
        }
        else
        {
            envAtReleaseStart = sustain;
        }

        double r = 1.0 - (tRel / release);
        double env = envAtReleaseStart * r;
        if (env < 0.0)
            env = 0.0;
        return env;
    }

    // ------------------------------------------------------------
    // ------------------------------------------------------------
    // **Action-8: Predictive Scoring (public DSP API)**
    // ------------------------------------------------------------
    // ------------------------------------------------------------

    // Predict listener position at t + τ
    juce::Point<float> predictListenerPosition(double horizonSeconds) const
    {
        auto u = computeUnitVector();
        double v = computeSpeed();

        double dx = v * static_cast<double>(u.x) * horizonSeconds;
        double dy = v * static_cast<double>(u.y) * horizonSeconds;

        return {
            listenerPos_.x + static_cast<float>(dx),
            listenerPos_.y + static_cast<float>(dy)
        };
    }

    // ------------------------------------------------------------
    // FIXED SIGNATURE — matches unit tests and specification
    // ------------------------------------------------------------
    double computePredictiveRetardedTime(double horizonSeconds,
                                        const juce::Point<float>& emitterPos) const
    {
        // First predict listener position at future time t + τ
        auto xL = predictListenerPosition(horizonSeconds);

        // Distance from predicted listener position to emitter
        double dx = static_cast<double>(emitterPos.x) - static_cast<double>(xL.x);
        double dy = static_cast<double>(emitterPos.y) - static_cast<double>(xL.y);
        double r  = std::sqrt(dx * dx + dy * dy);

        // Future listener time = t + τ
        double tFuture = timeSec_ + horizonSeconds;

        // Retarded time at predicted future state
        return tFuture - r / speedOfSound_;
    }

    // Full predictive score using horizons {0, H/2, H}
    double computePredictiveScoreForEmitter(const juce::Point<float>& emitterPos) const
    {
        const double H = predictiveHorizonSeconds_;
        const double horizons[3] = { 0.0, 0.5 * H, H };

        double best = -std::numeric_limits<double>::infinity();

        for (double tau : horizons)
        {
            // NOTE: signature fixed — tau first, emitter second
            double tRet = computePredictiveRetardedTime(tau, emitterPos);
            if (tRet > best)
                best = tRet;
        }

        return best;
    } 

    // ------------------------------------------------------------
    // State queries
    // ------------------------------------------------------------
    bool  isActive() const override         { return active_; }
    int   getNote() const noexcept override { return midiNote_; }
    float getCurrentLevel() const override  { return level_; }

private:
    // Phase III skeleton state
    double sampleRate_ = 48000.0;
    bool   active_     = false;
    int    midiNote_   = -1;
    float  level_      = 0.0f;

    // Action-1 state
    juce::Point<float> listenerPos_;
    double             timeSec_;
    float              speedNorm_;
    float              headingNorm_;

    // Action-2 flag
    bool enableTimeAccumulation_ = false;

    // Action-5 emitter field parameters (per voice instance)
    float densityNorm_     = 0.0f;  // ρ in [0,1]
    float orientationNorm_ = 0.0f;  // normalized orientation

    // ------------------------------------------------------------
    // Action-7 source parameters (per voice instance)
    // ------------------------------------------------------------
    double baseFrequencyHz_   = 220.0;
    double basePhaseRad_      = 0.0;
    double fieldPulseHz_      = 1.0;

    double adsrAttackSec_     = 0.01;
    double adsrDecaySec_      = 0.1;
    double adsrSustainLevel_  = 0.7;
    double adsrReleaseSec_    = 0.2;

    double noteOnTimeSec_     = 0.0;
    double noteOffTimeSec_    = std::numeric_limits<double>::infinity();

    // ------------------------------------------------------------
    // Action-3.1 / Action-5 / Action-6 constants
    // ------------------------------------------------------------
    static constexpr double vMax_          = 1.0;
    static constexpr double deltaParallel_ = 1.0;
    static constexpr double speedOfSound_  = 343.0;

    // ------------------------------------------------------------
    // **Action-8 constant**
    // ------------------------------------------------------------
    static constexpr double predictiveHorizonSeconds_ = 1.0; // H = 1s
};
