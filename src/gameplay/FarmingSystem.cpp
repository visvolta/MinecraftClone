#include "gameplay/FarmingSystem.h"

#include "Chunk.h"
#include "World.h"

#include <algorithm>
#include <cmath>

namespace mc::gameplay
{
FarmingSystem::FarmingSystem(std::uint64_t seed) noexcept
    : randomState_(seed == 0 ? 0x9E3779B97F4A7C15ULL : seed)
{
}

std::uint32_t FarmingSystem::nextRandom() noexcept
{
    randomState_ ^= randomState_ >> 12U;
    randomState_ ^= randomState_ << 25U;
    randomState_ ^= randomState_ >> 27U;
    return static_cast<std::uint32_t>(
        (randomState_ * 2685821657736338717ULL) >> 32U
    );
}

bool FarmingSystem::hasWater(
    World& world, int x, int y, int z) const
{
    for (int dz = -4; dz <= 4; ++dz)
    {
        for (int dx = -4; dx <= 4; ++dx)
        {
            if (world.getBlock(x + dx, y, z + dz) == BlockType::Water ||
                world.getBlock(x + dx, y + 1, z + dz) == BlockType::Water)
                return true;
        }
    }
    return false;
}

float FarmingSystem::cropGrowthChance(
    World& world, int x, int y, int z) const
{
    float growth = 1.0f;
    for (int dz = -1; dz <= 1; ++dz)
    {
        for (int dx = -1; dx <= 1; ++dx)
        {
            const auto soil = world.getBlockState(x + dx, y - 1, z + dz);
            if (soil.block() != BlockType::Farmland)
                continue;
            float contribution = soil.properties() > 0 ? 3.0f : 1.0f;
            if (dx != 0 || dz != 0)
                contribution *= 0.25f;
            growth += contribution;
        }
    }

    const BlockType crop = world.getBlock(x, y, z);
    const bool sameHorizontal =
        world.getBlock(x - 1, y, z) == crop ||
        world.getBlock(x + 1, y, z) == crop;
    const bool sameVertical =
        world.getBlock(x, y, z - 1) == crop ||
        world.getBlock(x, y, z + 1) == crop;
    const bool sameDiagonal =
        world.getBlock(x - 1, y, z - 1) == crop ||
        world.getBlock(x + 1, y, z - 1) == crop ||
        world.getBlock(x - 1, y, z + 1) == crop ||
        world.getBlock(x + 1, y, z + 1) == crop;
    if (sameDiagonal || (sameHorizontal && sameVertical))
        growth *= 0.5f;
    return growth;
}

void FarmingSystem::tick(World& world)
{
    for (Chunk* chunk : world.getChunkManager().getChunks())
    {
        if (chunk == nullptr)
            continue;
        const int originX = chunk->getChunkX() * Chunk::WIDTH;
        const int originZ = chunk->getChunkZ() * Chunk::DEPTH;
        for (int section = 0; section < Chunk::SECTION_COUNT; ++section)
        {
            if (chunk->isSectionEmpty(section))
                continue;
            for (int attempt = 0; attempt < 3; ++attempt)
            {
                const int x = originX + static_cast<int>(nextRandom() & 15U);
                const int y = section * Chunk::SECTION_HEIGHT +
                    static_cast<int>(nextRandom() & 15U);
                const int z = originZ + static_cast<int>(nextRandom() & 15U);
                const auto state = world.getBlockState(x, y, z);
                if (state.block() == BlockType::Farmland)
                {
                    const int moisture = state.properties() & 7;
                    if (hasWater(world, x, y, z))
                    {
                        if (moisture < 7)
                            world.setBlockAndMetadata(x, y, z, BlockType::Farmland, 7);
                    }
                    else if (moisture > 0)
                    {
                        world.setBlockAndMetadata(
                            x, y, z, BlockType::Farmland,
                            static_cast<std::uint8_t>(moisture - 1)
                        );
                    }
                    else if (!isCrop(world.getBlock(x, y + 1, z)))
                    {
                        world.setBlock(x, y, z, BlockType::Dirt);
                    }
                }
                else if (isCrop(state.block()) &&
                         state.properties() <
                             (state.block() == BlockType::Beetroots ? 3 : 7) &&
                         std::max(
                             world.getSkyLightLevel(x, y + 1, z),
                             world.getBlockLightLevel(x, y + 1, z)
                         ) >= 9)
                {
                    const float growth = cropGrowthChance(world, x, y, z);
                    const int bound = std::max(
                        1, static_cast<int>(std::floor(25.0f / growth)) + 1
                    );
                    if (nextRandom() % static_cast<std::uint32_t>(bound) == 0U)
                    {
                        world.setBlockAndMetadata(
                            x, y, z, state.block(),
                            static_cast<std::uint8_t>(state.properties() + 1)
                        );
                    }
                }
            }
        }
    }
}
}
