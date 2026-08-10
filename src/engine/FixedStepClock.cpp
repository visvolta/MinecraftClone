#include "engine/FixedStepClock.h"

#include <algorithm>
#include <stdexcept>

namespace mc::engine
{
FixedStepClock::FixedStepClock(
    double ticksPerSecond,
    int maximumTicksPerFrame)
    : maximumTicksPerFrame_(maximumTicksPerFrame)
{
    if (ticksPerSecond <= 0.0 || maximumTicksPerFrame <= 0)
        throw std::invalid_argument("Fixed-step clock values must be positive");
    tickSeconds_ = 1.0 / ticksPerSecond;
}

int FixedStepClock::advance(double frameSeconds) noexcept
{
    accumulator_ += std::clamp(frameSeconds, 0.0, 0.25);
    int ticks = 0;
    while (accumulator_ >= tickSeconds_ && ticks < maximumTicksPerFrame_)
    {
        accumulator_ -= tickSeconds_;
        ++ticks;
        ++tickCount_;
    }
    if (ticks == maximumTicksPerFrame_ && accumulator_ >= tickSeconds_)
        accumulator_ = 0.0;
    return ticks;
}

float FixedStepClock::partialTick() const noexcept
{
    return static_cast<float>(accumulator_ / tickSeconds_);
}

std::uint64_t FixedStepClock::tickCount() const noexcept
{
    return tickCount_;
}

double FixedStepClock::tickSeconds() const noexcept
{
    return tickSeconds_;
}
}
