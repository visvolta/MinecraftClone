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

// EntityBodyHelper.computeAngleWithBound: return angle2 moved just far
// enough that |angle1 - result| <= maxDelta.
[[nodiscard]] inline float computeAngleWithBound(
    float angle1,
    float angle2,
    float maxDelta) noexcept
{
    float delta = wrapDegrees(angle1 - angle2);
    if (delta < -maxDelta)
        delta = -maxDelta;
    if (delta >= maxDelta)
        delta = maxDelta;
    return angle1 - delta;
}

inline void wrapAnglePair(float& current, float& previous) noexcept
{
    while (current - previous < -180.0f)
        previous -= 360.0f;
    while (current - previous >= 180.0f)
        previous += 360.0f;
}

// EntityMob.isValidLightLevel, factored for tests.
// skyLight is EnumSkyBlock.SKY (raw). neighborLight is
// World.getLightFromNeighbors (time-adjusted combined light).
// skyRoll is rand.nextInt(32); neighborRoll is rand.nextInt(8).
[[nodiscard]] inline bool vanillaHostileLightAllowsSpawn(
    int skyLight,
    int neighborLight,
    int skyRoll,
    int neighborRoll) noexcept
{
    if (skyLight > skyRoll)
        return false;
    return neighborLight <= neighborRoll;
}
}
