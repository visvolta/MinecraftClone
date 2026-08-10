#pragma once

#include "Block.h"

#include <algorithm>
#include <cstdint>

namespace FluidState
{
constexpr int SourceLevel = 0;
constexpr int MaximumHorizontalLevel = 7;
constexpr int FallingFlag = 8;
constexpr int MaximumStoredLevel = 15;

[[nodiscard]] constexpr bool isFalling(std::uint8_t level) noexcept
{
    return level >= FallingFlag;
}

[[nodiscard]] constexpr int effectiveLevel(std::uint8_t level) noexcept
{
    return isFalling(level) ? SourceLevel : static_cast<int>(level);
}

[[nodiscard]] constexpr float airFraction(std::uint8_t level) noexcept
{
    const int surfaceLevel = isFalling(level)
        ? SourceLevel
        : static_cast<int>(level);
    return static_cast<float>(surfaceLevel + 1) / 9.0f;
}

[[nodiscard]] constexpr int spreadStep(BlockType liquid) noexcept
{
    return liquid == BlockType::Lava ? 2 : 1;
}

[[nodiscard]] constexpr int tickRate(BlockType liquid) noexcept
{
    return liquid == BlockType::Lava ? 30 : 5;
}

[[nodiscard]] constexpr std::uint8_t clampLevel(int level) noexcept
{
    return static_cast<std::uint8_t>(
        std::clamp(level, SourceLevel, MaximumStoredLevel)
    );
}
}
