#pragma once

#include <algorithm>
#include <cmath>

namespace mc::entity
{
[[nodiscard]] inline float wrapDegrees(float value) noexcept
{
    value = std::fmod(value, 360.0f);
    if (value >= 180.0f)
        value -= 360.0f;
    if (value < -180.0f)
        value += 360.0f;
    return value;
}

[[nodiscard]] inline float toRadians(float degrees) noexcept
{
    return degrees * 0.017453292f;
}

[[nodiscard]] inline float toDegrees(float radians) noexcept
{
    return radians * 57.295776f;
}

[[nodiscard]] inline int floorInt(double value) noexcept
{
    return static_cast<int>(std::floor(value));
}

[[nodiscard]] inline int ceilInt(float value) noexcept
{
    return static_cast<int>(std::ceil(value));
}

[[nodiscard]] inline float approachDegrees(
    float current,
    float target,
    float maxChange) noexcept
{
    float delta = wrapDegrees(target - current);
    delta = std::clamp(delta, -maxChange, maxChange);
    return current + delta;
}
}
