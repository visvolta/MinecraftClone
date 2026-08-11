#include "worldgen/WorldGenerationContext.h"

#include "Chunk.h"

#include <utility>
#include <algorithm>
#include <cstdint>

std::size_t WorldGenerationContext::FeaturePositionHash::operator()(
    const FeaturePosition& position) const noexcept
{
    // A stable integer mix handles negative world coordinates without
    // relying on implementation-defined shifts.
    std::uint64_t hash = static_cast<std::uint32_t>(position.x);
    hash ^= static_cast<std::uint64_t>(
        static_cast<std::uint32_t>(position.z)
    ) << 32U;
    hash ^= static_cast<std::uint64_t>(
        static_cast<std::uint32_t>(position.y)
    ) * 0x9E3779B97F4A7C15ULL;
    hash ^= hash >> 30U;
    hash *= 0xBF58476D1CE4E5B9ULL;
    hash ^= hash >> 27U;
    return static_cast<std::size_t>(hash ^ (hash >> 31U));
}

WorldGenerationContext::WorldGenerationContext(
    Chunk& targetChunk,
    FallbackSampler fallbackSampler,
    HeightSampler heightSampler,
    ClimateSampler climateSampler)
    : targetChunk_(targetChunk),
      fallbackSampler_(std::move(fallbackSampler)),
      heightSampler_(std::move(heightSampler)),
      climateSampler_(std::move(climateSampler))
{
}

BlockType WorldGenerationContext::getBlock(
    int worldX,
    int worldY,
    int worldZ) const
{
    return getBlockState(worldX, worldY, worldZ).block();
}

mc::content::BlockState WorldGenerationContext::getBlockState(
    int worldX,
    int worldY,
    int worldZ) const
{
    if (worldY < 0 || worldY >= Chunk::HEIGHT)
    {
        return {};
    }

    if (isolatedFeatureActive_)
    {
        const auto staged = stagedFeatureBlocks_.find(
            FeaturePosition{worldX, worldY, worldZ}
        );
        if (staged != stagedFeatureBlocks_.end())
            return staged->second;

        // Population is replayed separately for every target chunk. Reading
        // the terrain snapshot here gives every replay exactly the same
        // validation input, including when a tree crosses a chunk boundary.
        return fallbackSampler_
            ? mc::content::BlockState(fallbackSampler_(worldX, worldY, worldZ))
            : mc::content::BlockState{};
    }

    if (isInsideTarget(worldX, worldY, worldZ))
    {
        return targetChunk_.getBlockState(
            worldX - targetChunk_.getWorldOriginX(),
            worldY,
            worldZ - targetChunk_.getWorldOriginZ()
        );
    }

    return fallbackSampler_
        ? mc::content::BlockState(fallbackSampler_(worldX, worldY, worldZ))
        : mc::content::BlockState{};
}

bool WorldGenerationContext::isInsideTarget(
    int worldX,
    int worldY,
    int worldZ) const noexcept
{
    return worldY >= 0 &&
           worldY < Chunk::HEIGHT &&
           worldX >= targetChunk_.getWorldOriginX() &&
           worldX < targetChunk_.getWorldOriginX() + Chunk::WIDTH &&
           worldZ >= targetChunk_.getWorldOriginZ() &&
           worldZ < targetChunk_.getWorldOriginZ() + Chunk::DEPTH;
}

bool WorldGenerationContext::setBlock(
    int worldX,
    int worldY,
    int worldZ,
    BlockType block)
{
    return setBlockState(
        worldX, worldY, worldZ, mc::content::BlockState(block)
    );
}

bool WorldGenerationContext::setBlockState(
    int worldX,
    int worldY,
    int worldZ,
    mc::content::BlockState state)
{
    if (isolatedFeatureActive_)
    {
        if (worldY < 0 || worldY >= Chunk::HEIGHT)
            return false;
        stagedFeatureBlocks_.insert_or_assign(
            FeaturePosition{worldX, worldY, worldZ}, state
        );
        return true;
    }

    if (!isInsideTarget(worldX, worldY, worldZ))
    {
        return false;
    }

    return targetChunk_.setBlockState(
        worldX - targetChunk_.getWorldOriginX(),
        worldY,
        worldZ - targetChunk_.getWorldOriginZ(),
        state
    );
}

void WorldGenerationContext::beginIsolatedFeature()
{
    stagedFeatureBlocks_.clear();
    isolatedFeatureActive_ = true;
}

void WorldGenerationContext::finishIsolatedFeature(bool commit)
{
    if (commit)
    {
        for (const auto& [position, state] : stagedFeatureBlocks_)
        {
            if (!isInsideTarget(position.x, position.y, position.z))
                continue;
            targetChunk_.setBlockState(
                position.x - targetChunk_.getWorldOriginX(),
                position.y,
                position.z - targetChunk_.getWorldOriginZ(),
                state
            );
        }
    }

    stagedFeatureBlocks_.clear();
    isolatedFeatureActive_ = false;
}


int WorldGenerationContext::getHeightValue(int worldX, int worldZ) const
{
    if (heightSampler_)
        return heightSampler_(worldX, worldZ);

    for (int y = Chunk::HEIGHT - 1; y >= 0; --y)
    {
        const BlockType block = getBlock(worldX, y, worldZ);
        if (block != BlockType::Air && !isLiquid(block) && !isLeaf(block))
            return y + 1;
    }

    return 0;
}

ClimateSample WorldGenerationContext::sampleClimate(
    int worldX,
    int worldZ) const
{
    if (climateSampler_)
        return climateSampler_(worldX, worldZ);

    if (worldX >= targetChunk_.getWorldOriginX() &&
        worldX < targetChunk_.getWorldOriginX() + Chunk::WIDTH &&
        worldZ >= targetChunk_.getWorldOriginZ() &&
        worldZ < targetChunk_.getWorldOriginZ() + Chunk::DEPTH)
    {
        const int localX = worldX - targetChunk_.getWorldOriginX();
        const int localZ = worldZ - targetChunk_.getWorldOriginZ();
        const double temperature = targetChunk_.getTemperature(localX, localZ);
        const double humidity = targetChunk_.getHumidity(localX, localZ);
        return {
            temperature,
            humidity,
            targetChunk_.getBiome(localX, localZ)
        };
    }

    // Population is replayed per target chunk. Outside-target climate data is
    // not stored in the fallback sampler, so use the nearest target column.
    const int localX = std::clamp(
        worldX - targetChunk_.getWorldOriginX(),
        0,
        Chunk::WIDTH - 1
    );
    const int localZ = std::clamp(
        worldZ - targetChunk_.getWorldOriginZ(),
        0,
        Chunk::DEPTH - 1
    );
    const double temperature = targetChunk_.getTemperature(localX, localZ);
    const double humidity = targetChunk_.getHumidity(localX, localZ);
    return {
        temperature,
        humidity,
        targetChunk_.getBiome(localX, localZ)
    };
}
