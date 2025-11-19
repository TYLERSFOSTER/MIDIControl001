#pragma once

#include "dsp/BaseVoice.h"
#include "params/ParameterSnapshot.h"

#include <cmath>
#include <atomic>
#include <limits>  // for std::numeric_limits

// We need juce::Point; pull in the graphics module.
#include <juce_graphics/juce_graphics.h>

// ============================================================
// VoiceDopp — Phase IV Action 1–5
// ------------------------------------------------------------
// Action 1: Kinematic API (position, time, listener controls)
// Action 2: Time accumulator (feature-gated, still OFF)
// Action 3.1: Heading + speed mapping (pure math only)
// Action 4: Listener trajectory integration (gated)
// Action 5: Emitter lattice construction (pure math only)
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
    }

    // ------------------------------------------------------------
    // noteOff()
    // ------------------------------------------------------------
    void noteOff() override
    {
        active_ = false;
        level_  = 0.0f;
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

        // Still silent for Actions 1–5
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
    // Action-3.1 / Action-5 constants
    // ------------------------------------------------------------
    static constexpr double vMax_          = 1.0;
    static constexpr double deltaParallel_ = 1.0; // Δ∥
};
