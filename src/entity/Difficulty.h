#pragma once

#include <cstdint>

namespace mc::entity
{
enum class Difficulty : std::uint8_t
{
    Peaceful = 0,
    Easy = 1,
    Normal = 2,
    Hard = 3
};

[[nodiscard]] inline int difficultyId(Difficulty difficulty) noexcept
{
    return static_cast<int>(difficulty);
}
}
