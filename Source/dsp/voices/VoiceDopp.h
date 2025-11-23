#pragma once

#include "dsp/BaseVoice.h"
#include "params/ParameterSnapshot.h"

#include <cmath>
#include <atomic>
#include <limits>  // for std::numeric_limits
#include <vector>

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
        (void)velocity;

        // ============================================================
        // A10-1: baseFrequencyHz_
        // ============================================================
        if (pitchFromMidi_)
        {
            // MIDI → frequency relative to A4 defined by snapshot.oscFreq
            const double fA4 = snapshot.oscFreq;
            baseFrequencyHz_ =
                fA4 * std::pow(2.0, (static_cast<double>(midiNote) - 69.0) / 12.0);
        }
        else
        {
            // Test-required behavior: snapshot.oscFreq must pass through unchanged
            baseFrequencyHz_ = snapshot.oscFreq;
        }

        // Envelope globals
        adsrAttackSec_  = snapshot.envAttack;
        adsrReleaseSec_ = snapshot.envRelease;

        // ============================================================
        // Phase IV — CC sampling at note-on (Spec §1)
        // These CC values were cached earlier in handleController().
        // Only overwrite defaults if the CC was actually received.
        // ============================================================

        // --- CC4: field pulse frequency μ_pulse ---
        if (hasCC4_)
        {
            fieldPulseHz_ = mapFieldPulseNormToHz(cc4FieldPulseNorm_);
        }

        // --- CC7: lattice orientation φ ---
        if (hasCC7_)
        {
            orientationNorm_ = cc7OrientationNorm_;
        }

        // --- CC8: lattice density ρ ---
        if (hasCC8_)
        {
            densityNorm_ = cc8DensityNorm_;
        }

        // ============================================================
        // Standard VoiceDopp activation
        // ============================================================
        midiNote_ = midiNote;
        active_   = true;
        level_    = 1.0f;

        listenerPos_ = { 0.0f, 0.0f };
        timeSec_     = 0.0;

        // For now ADSR timing stays mathematical (per your A7 spec):
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
        // We must preserve this even when audio is disabled.
        // ======================================
        const double sr = sampleRate_;
        const double dtBlock = (sr > 0.0)
                                 ? static_cast<double>(numSamples) / sr
                                 : 0.0;

        // snapshot "start of block" state for per-sample synthesis
        const double tStart = timeSec_;
        const auto   posStart = listenerPos_;

        // ============================================================
        // FIX 1 — advance time when audioEnabled_ (silent bug fix)
        // ============================================================
        if ((audioEnabled_ || enableTimeAccumulation_) && sr > 0.0)
        {
            timeSec_ += dtBlock;

            // Listener kinematics only if accumulation is enabled
            if (enableTimeAccumulation_)
            {
                double speed = computeSpeed();
                auto   uvec  = computeUnitVector();

                double vx = speed * static_cast<double>(uvec.x);
                double vy = speed * static_cast<double>(uvec.y);

                listenerPos_.x += static_cast<float>(vx * dtBlock);
                listenerPos_.y += static_cast<float>(vy * dtBlock);
            }
        }

        // ======================================
        // Action-10.5 gate: keep legacy silence unless enabled
        // ======================================
        if (!audioEnabled_)
        {
            std::fill(buffer, buffer + numSamples, 0.0f);
            return;
        }

        // ======================================
        // Audible Doppler synthesis path (math already implemented)
        // ======================================

        // Choose best emitter in a small default window.
        // Window size is a tunable constant; safe for tests because gate is off.
        const int kMin = -latticeKRadius_;
        const int kMax =  latticeKRadius_;
        const int mMin = -latticeMRadius_;
        const int mMax =  latticeMRadius_;

        auto best = findBestEmitterInWindow(kMin, kMax, mMin, mMax);
        auto emitterPos = best.position;

        // Instantaneous listener velocity for local per-sample prediction
        const double speed = computeSpeed();
        const auto   uvec  = computeUnitVector();
        const double vx = speed * static_cast<double>(uvec.x);
        const double vy = speed * static_cast<double>(uvec.y);

        // Synthesize per sample
        for (int i = 0; i < numSamples; ++i)
        {
            const double tSample =
                (sr > 0.0)
                    ? (tStart + static_cast<double>(i) / sr)
                    : tStart;

            const juce::Point<float> posSample {
                posStart.x + static_cast<float>(vx * (static_cast<double>(i) / sr)),
                posStart.y + static_cast<float>(vy * (static_cast<double>(i) / sr))
            };

            // Distance r_i(t)
            const double dx = static_cast<double>(emitterPos.x) - static_cast<double>(posSample.x);
            const double dy = static_cast<double>(emitterPos.y) - static_cast<double>(posSample.y);
            const double r  = std::sqrt(dx*dx + dy*dy);

            // Retarded time t_ret = t - r/c
            const double tRet = tSample - r / speedOfSound_;

            // Source components at retarded time
            const double carrier = evalCarrierAtRetardedTime(tRet);
            const double env     = evalAdsrAtRetardedTime(tRet);
            const double pulse   = evalFieldPulseAtRetardedTime(tRet);

            // Simple attenuation kernel (local-only, does not affect predictive score yet)
            const double atten = evalAttenuationKernel(r);

            const double sample = carrier * env * pulse * atten;

            buffer[i] += static_cast<float>(sample);
        }
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
    // CC routing (Spec §1)
    // VoiceManager already dispatches CC to all voices.
    // We cache CC4/7/8 for note-on sampling, and apply CC5/6 live.
    // ------------------------------------------------------------
    void handleController(int cc, float norm) override
    {
        switch (cc)
        {
            case 4: // CC4: field pulse frequency (sampled at note-on)
                cc4FieldPulseNorm_ = norm;
                hasCC4_ = true;
                break;

            case 5: // CC5: listener speed scalar s(t), continuous/blockwise
                speedNorm_ = norm;
                break;

            case 6: // CC6: listener heading η(t), continuous/blockwise
                headingNorm_ = norm;
                break;

            case 7: // CC7: lattice orientation φ (sampled at note-on)
                cc7OrientationNorm_ = norm;
                hasCC7_ = true;
                break;

            case 8: // CC8: lattice density ρ (sampled at note-on)
                cc8DensityNorm_ = norm;
                hasCC8_ = true;
                break;

            default:
                break; // ignore other CC in VoiceDopp for now
        }
    }

    // ------------------------------------------------------------
    // NEW: Action-9 diagnostic exposure (public API)
    // ------------------------------------------------------------
    /** Returns the instantaneous listener velocity vector v(t). */
    juce::Point<float> getListenerVelocity() const noexcept
    {
        double v = computeSpeed();
        auto   u = computeUnitVector();

        return {
            static_cast<float>(v * static_cast<double>(u.x)),
            static_cast<float>(v * static_cast<double>(u.y))
        };
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
    // Action-10.5: Audio synthesis gate (default OFF)
    // ------------------------------------------------------------
    void setAudioSynthesisEnabled(bool shouldEnable) noexcept override
    {
        audioEnabled_ = shouldEnable;
    }

    bool isAudioSynthesisEnabledForTest() const noexcept
    {
        return audioEnabled_;
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
        const double dPerp = computeDeltaPerp();
        const double dPar  = computeDeltaParallel();

        const auto n = computeEmitterNormal();
        const auto b = computeEmitterTangent();

        const double nx = static_cast<double>(n.x);
        const double ny = static_cast<double>(n.y);
        const double bx = static_cast<double>(b.x);
        const double by = static_cast<double>(b.y);

        double x = 0.0;
        double y = 0.0;

        // ============================================================
        // FIX: rho=0 => dPerp=inf. Single-line case must not do 0*inf.
        // Only k=0 is meaningful; all k!=0 collapse to "far away".
        // ============================================================
        if (std::isfinite(dPerp))
        {
            x += static_cast<double>(k) * dPerp * nx;
            y += static_cast<double>(k) * dPerp * ny;
        }
        else
        {
            // single-line lattice: ignore normal offsets
            // (k term contributes 0)
        }

        x += static_cast<double>(m) * dPar * bx;
        y += static_cast<double>(m) * dPar * by;

        return { static_cast<float>(x), static_cast<float>(y) };
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

    // ------------------------------------------------------------
    // A10-1: Per-voice synthesis params pipeline
    // Called by VoiceManager::startBlock() like VoiceA::updateParams()
    // ------------------------------------------------------------
    void updateParams(const VoiceParams& vp)
    {
        if (!pitchFromMidi_)
            baseFrequencyHz_ = vp.oscFreq;

        adsrAttackSec_   = vp.envAttack;
        adsrReleaseSec_  = vp.envRelease;
    }

    // ------------------------------------------------------------
    // A10-1: minimal public getters for tests
    // ------------------------------------------------------------
    double getBaseFrequencyHzForTest() const noexcept  { return baseFrequencyHz_; }
    double getAdsrAttackSecForTest() const noexcept    { return adsrAttackSec_; }
    double getAdsrReleaseSecForTest() const noexcept   { return adsrReleaseSec_; }

    // === NEW for Action10.1 =========================================
    double getBaseFrequencyForTest() const noexcept { return baseFrequencyHz_; }
    double getAttackForTest() const noexcept        { return adsrAttackSec_; }
    double getReleaseForTest() const noexcept       { return adsrReleaseSec_; }
    
    // ------------------------------------------------------------
    // Action-9: Lattice window sampling + best-emitter selection
    // ------------------------------------------------------------
    struct EmitterCandidate
    {
        juce::Point<float> position {};  // world-space emitter position
        int k = 0;                       // lattice index k (normal direction)
        int m = 0;                       // lattice index m (tangent direction)
        double score = 0.0;              // predictive score (Action-8)
    };

    // Scan a finite (k,m) window, compute predictive scores, and
    // return the best emitter according to Action-8.
    //
    // If the window is "empty" (kMin > kMax or mMin > mMax),
    // this returns a default-constructed candidate with score = 0.
    EmitterCandidate findBestEmitterInWindow(int kMin, int kMax,
                                             int mMin, int mMax) const
    {
        EmitterCandidate best;
        bool hasBest = false;

        if (kMin > kMax || mMin > mMax)
            return best; // default (score = 0.0)

        for (int k = kMin; k <= kMax; ++k)
        {
            for (int m = mMin; m <= mMax; ++m)
            {
                auto pos = computeEmitterPosition(k, m);
                if (!std::isfinite(pos.x) || !std::isfinite(pos.y))
                    continue;

                double s = computePredictiveScoreForEmitter(pos);

                if (!hasBest || s > best.score)
                {
                    best.position = pos;
                    best.k        = k;
                    best.m        = m;
                    best.score    = s;
                    hasBest       = true;
                }
            }
        }

        return best;
    }

    void setPitchFromMidi(bool b) noexcept
    {
        pitchFromMidi_ = b;
    }

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
    // Phase IV CC cache for VoiceDopp (Spec §1)
    // CC4/7/8 sampled at note-on. CC5/6 are continuous/blockwise.
    // ------------------------------------------------------------
    float cc4FieldPulseNorm_   = 0.0f;
    float cc7OrientationNorm_  = 0.0f;
    float cc8DensityNorm_      = 0.0f;

    bool  hasCC4_ = false;
    bool  hasCC7_ = false;
    bool  hasCC8_ = false;

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

    // ------------------------------------------------------------
    // Action-10.5 local attenuation kernel
    // w(r) = exp(-alpha r) / max(r, r_min)
    // This is *audio-only* for now; predictive score integration is A10-4.
    // ------------------------------------------------------------
    double evalAttenuationKernel(double r) const noexcept
    {
        const double rSafe = (r < attenuationRMin_) ? attenuationRMin_ : r;
        return std::exp(-attenuationAlpha_ * rSafe) / rSafe;
    }

    // Map CC4 normalized [0,1] to a usable pulse frequency in Hz.
    // Spec doesn't fix a range, so choose a sane exponential range.
    double mapFieldPulseNormToHz(float norm) const noexcept
    {
        const double lo = 0.1;   // Hz
        const double hi = 20.0;  // Hz
        const double n  = juce::jlimit(0.0, 1.0, static_cast<double>(norm));

        const double logLo = std::log(lo);
        const double logHi = std::log(hi);
        return std::exp(logLo + (logHi - logLo) * n);
    }

    // Small default lattice window for audio (tune later)
    static constexpr int latticeKRadius_ = 2;
    static constexpr int latticeMRadius_ = 4;

    static constexpr double attenuationAlpha_ = 0.05;
    static constexpr double attenuationRMin_  = 0.25;

    bool audioEnabled_ = false;

    bool pitchFromMidi_ = false;
};
