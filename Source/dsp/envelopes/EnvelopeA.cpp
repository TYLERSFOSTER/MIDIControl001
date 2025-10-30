#include <juce_core/juce_core.h>
#include "EnvelopeA.h"
#include <cmath>

void EnvelopeA::prepare(double sr)
{
    sampleRate_ = sr;
    setAttack(0.01f);
    setRelease(0.2f);
}

void EnvelopeA::setAttack(float seconds)
{
    attackInc_ = (seconds > 0.0f) ? (1.0 / (seconds * sampleRate_)) : 1.0;
}

void EnvelopeA::setRelease(float seconds)
{
    releaseSeconds_ = std::max(0.0f, seconds);

    if (releaseSeconds_ == 0.0f)
    {
        releaseCoef_ = 0.0;
        return;
    }

    const double N = releaseSeconds_ * sampleRate_;
    releaseCoef_ = std::exp(std::log(1e-5) / N); // reach -100 dB in N samples
}

void EnvelopeA::noteOn()
{
    state_ = State::Attack;
    level_ = 0.0;
    releaseSamples_ = 0;
}

void EnvelopeA::noteOff()
{
    if (state_ != State::Idle && state_ != State::Release)
    {
        DBG("Release start level: " << level_ << " coef: " << releaseCoef_);
        state_ = State::Release;
        releaseStartLevel_ = level_;
        releaseSamples_ = 0;
    }
}

float EnvelopeA::nextSample()
{
    static int counter = 0;
    counter++;

    if (state_ == State::Release && counter % 480 == 0)
        DBG("EnvelopeA release level=" << level_);

    switch (state_)
    {
        case State::Attack:
            level_ += attackInc_;
            if (level_ >= 1.0)
            {
                level_ = 1.0;
                state_ = State::Sustain;
            }
            break;

        case State::Sustain:
            // sustain holds level
            break;

        case State::Release:
        {
            ++releaseSamples_;
            level_ *= releaseCoef_;

            const uint64_t releaseSamplesTarget =
                static_cast<uint64_t>(releaseSeconds_ * sampleRate_);

            if (level_ <= 1e-5 || releaseSamples_ >= releaseSamplesTarget)
            {
                level_ = 0.0;
                state_ = State::Idle;
                DBG("EnvelopeA finished, level=" << level_);
            }
            break;
        }

        case State::Idle:
        default:
            break;
    }

    return static_cast<float>(level_);
}

bool EnvelopeA::isActive() const
{
    if (state_ == State::Idle)
        DBG("EnvelopeA now Idle");
    return state_ != State::Idle;
}
