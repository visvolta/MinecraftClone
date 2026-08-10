#pragma once

#include <cstdint>

namespace mc::engine
{
class FixedStepClock
{
public:
    explicit FixedStepClock(
        double ticksPerSecond = 20.0,
        int maximumTicksPerFrame = 5
    );

    [[nodiscard]] int advance(double frameSeconds) noexcept;
    [[nodiscard]] float partialTick() const noexcept;
    [[nodiscard]] std::uint64_t tickCount() const noexcept;
    [[nodiscard]] double tickSeconds() const noexcept;

private:
    double tickSeconds_ = 1.0 / 20.0;
    double accumulator_ = 0.0;
    int maximumTicksPerFrame_ = 5;
    std::uint64_t tickCount_ = 0;
};
}
