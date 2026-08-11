#pragma once

#include <cstdint>
#include <glm/vec3.hpp>

[[nodiscard]] inline std::uint64_t blockModelSeed(
    int x, int y, int z) noexcept
{
    std::int64_t value =
        static_cast<std::int64_t>(x) * 3129871LL ^
        static_cast<std::int64_t>(z) * 116129781LL ^
        static_cast<std::int64_t>(y);
    value = value * value * 42317861LL + value * 11LL;
    return static_cast<std::uint64_t>(value >> 16);
}

[[nodiscard]] inline std::uint64_t blockModelSeed(
    const glm::ivec3& position) noexcept
{
    return blockModelSeed(position.x, position.y, position.z);
}
