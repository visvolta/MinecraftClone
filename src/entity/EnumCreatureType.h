#pragma once

#include <cstdint>

namespace mc::entity
{
enum class EnumCreatureType : std::uint8_t
{
    Monster,
    Creature,
    Ambient,
    WaterCreature
};

[[nodiscard]] inline int maxNumberOfCreature(EnumCreatureType type) noexcept
{
    switch (type)
    {
        case EnumCreatureType::Monster: return 70;
        case EnumCreatureType::Creature: return 10;
        case EnumCreatureType::Ambient: return 15;
        case EnumCreatureType::WaterCreature: return 5;
    }
    return 0;
}

[[nodiscard]] inline bool isPeacefulCreature(EnumCreatureType type) noexcept
{
    return type != EnumCreatureType::Monster;
}

[[nodiscard]] inline bool isAnimal(EnumCreatureType type) noexcept
{
    return type == EnumCreatureType::Creature;
}
}
