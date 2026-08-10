#include "worldgen/WorldGenerationContext.h"

#include "Chunk.h"

#include <utility>
#include <algorithm>

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
    if (worldY < 0 || worldY >= Chunk::HEIGHT)
    {
        return BlockType::Air;
    }

    if (isInsideTarget(worldX, worldY, worldZ))
    {
        return targetChunk_.getBlock(
            worldX - targetChunk_.getWorldOriginX(),
            worldY,
            worldZ - targetChunk_.getWorldOriginZ()
        );
    }

    return fallbackSampler_
        ? fallbackSampler_(worldX, worldY, worldZ)
        : BlockType::Air;
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
    if (!isInsideTarget(worldX, worldY, worldZ))
    {
        return false;
    }

    return targetChunk_.setBlock(
        worldX - targetChunk_.getWorldOriginX(),
        worldY,
        worldZ - targetChunk_.getWorldOriginZ(),
        block
    );
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
