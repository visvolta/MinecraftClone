#pragma once

#include <cstdint>

class World;

namespace mc::gameplay
{
class FarmingSystem
{
public:
    explicit FarmingSystem(std::uint64_t seed) noexcept;
    void tick(World& world);

private:
    std::uint64_t randomState_;

    [[nodiscard]] std::uint32_t nextRandom() noexcept;
    [[nodiscard]] bool hasWater(World& world, int x, int y, int z) const;
    [[nodiscard]] float cropGrowthChance(
        World& world, int x, int y, int z
    ) const;
};
}
