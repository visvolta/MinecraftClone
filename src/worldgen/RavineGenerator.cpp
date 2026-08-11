#include "worldgen/RavineGenerator.h"

#include "Block.h"
#include "Chunk.h"
#include "worldgen/JavaRandom.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <numbers>

namespace
{
std::int64_t makeOdd(std::int64_t value)
{
    return value / 2LL * 2LL + 1LL;
}

std::int64_t ravineSeed(
    int chunkX,
    std::int64_t xMultiplier,
    int chunkZ,
    std::int64_t zMultiplier,
    std::int64_t worldSeed)
{
    std::uint64_t value = static_cast<std::uint64_t>(
        static_cast<std::int64_t>(chunkX)) *
        static_cast<std::uint64_t>(xMultiplier);
    value += static_cast<std::uint64_t>(
        static_cast<std::int64_t>(chunkZ)) *
        static_cast<std::uint64_t>(zMultiplier);
    value ^= static_cast<std::uint64_t>(worldSeed);
    return std::bit_cast<std::int64_t>(value);
}

bool canCarve(BlockType block)
{
    return block == BlockType::Stone || block == BlockType::Dirt ||
           block == BlockType::Grass || block == BlockType::Gravel ||
           block == BlockType::Sand || block == BlockType::Snow;
}
}

RavineGenerator::RavineGenerator(std::int64_t worldSeed, int range)
    : worldSeed_(worldSeed), range_(std::max(1, range))
{
}

void RavineGenerator::generate(Chunk& targetChunk) const
{
    JavaRandom seedRandom(worldSeed_);
    const std::int64_t xMultiplier = makeOdd(seedRandom.nextLong());
    const std::int64_t zMultiplier = makeOdd(seedRandom.nextLong());
    for (int sourceX = targetChunk.getChunkX() - range_;
         sourceX <= targetChunk.getChunkX() + range_; ++sourceX)
    {
        for (int sourceZ = targetChunk.getChunkZ() - range_;
             sourceZ <= targetChunk.getChunkZ() + range_; ++sourceZ)
        {
            JavaRandom random(ravineSeed(
                sourceX, xMultiplier, sourceZ, zMultiplier, worldSeed_));
            generateFrom(random, sourceX, sourceZ, targetChunk);
        }
    }
}

void RavineGenerator::generateFrom(
    JavaRandom& random,
    int sourceChunkX,
    int sourceChunkZ,
    Chunk& targetChunk) const
{
    // MapGenRavine performs exactly one 1-in-50 start test per source chunk.
    if (random.nextInt(50) != 0)
        return;

    double x = sourceChunkX * 16.0 + random.nextInt(16);
    double y = random.nextInt(random.nextInt(40) + 8) + 20.0;
    double z = sourceChunkZ * 16.0 + random.nextInt(16);
    float yaw = random.nextFloat() * std::numbers::pi_v<float> * 2.0f;
    float pitch = (random.nextFloat() - 0.5f) / 4.0f;
    const float width = (random.nextFloat() * 2.0f + random.nextFloat()) * 2.0f;
    JavaRandom pathRandom(random.nextLong());
    const int maximumSteps = range_ * 16 - 16 -
        pathRandom.nextInt(std::max(1, range_ * 4));
    float yawVelocity = 0.0f;
    float pitchVelocity = 0.0f;

    const double targetCentreX = targetChunk.getWorldOriginX() + 8.0;
    const double targetCentreZ = targetChunk.getWorldOriginZ() + 8.0;
    for (int step = 0; step < maximumSteps; ++step)
    {
        const double horizontalRadius = 1.5 +
            std::sin(static_cast<double>(step) * std::numbers::pi /
                     static_cast<double>(maximumSteps)) * width;
        const double verticalRadius = horizontalRadius * 3.0;
        const float pitchCos = std::cos(pitch);
        x += std::cos(yaw) * pitchCos;
        y += std::sin(pitch);
        z += std::sin(yaw) * pitchCos;
        pitch *= 0.7f;
        pitch += pitchVelocity * 0.05f;
        yaw += yawVelocity * 0.05f;
        pitchVelocity = pitchVelocity * 0.8f +
            (pathRandom.nextFloat() - pathRandom.nextFloat()) *
                pathRandom.nextFloat() * 2.0f;
        yawVelocity = yawVelocity * 0.5f +
            (pathRandom.nextFloat() - pathRandom.nextFloat()) *
                pathRandom.nextFloat() * 4.0f;
        if (pathRandom.nextInt(4) == 0)
            continue;

        const double remaining = maximumSteps - step;
        const double dx = x - targetCentreX;
        const double dz = z - targetCentreZ;
        const double reach = width + 18.0;
        if (dx * dx + dz * dz - remaining * remaining > reach * reach)
            return;
        if (x < targetCentreX - 16.0 - horizontalRadius * 2.0 ||
            z < targetCentreZ - 16.0 - horizontalRadius * 2.0 ||
            x > targetCentreX + 16.0 + horizontalRadius * 2.0 ||
            z > targetCentreZ + 16.0 + horizontalRadius * 2.0)
            continue;

        const int minimumX = std::clamp(
            static_cast<int>(std::floor(x - horizontalRadius)) -
                targetChunk.getWorldOriginX() - 1, 0, 15);
        const int maximumX = std::clamp(
            static_cast<int>(std::floor(x + horizontalRadius)) -
                targetChunk.getWorldOriginX() + 1, 0, 16);
        const int minimumZ = std::clamp(
            static_cast<int>(std::floor(z - horizontalRadius)) -
                targetChunk.getWorldOriginZ() - 1, 0, 15);
        const int maximumZ = std::clamp(
            static_cast<int>(std::floor(z + horizontalRadius)) -
                targetChunk.getWorldOriginZ() + 1, 0, 16);
        const int minimumY = std::clamp(
            static_cast<int>(std::floor(y - verticalRadius)) - 1, 1, 248);
        const int maximumY = std::clamp(
            static_cast<int>(std::floor(y + verticalRadius)) + 1, 1, 248);

        for (int localX = minimumX; localX < maximumX; ++localX)
        {
            const double nx = (targetChunk.getWorldOriginX() + localX + 0.5 - x) /
                horizontalRadius;
            for (int localZ = minimumZ; localZ < maximumZ; ++localZ)
            {
                const double nz = (targetChunk.getWorldOriginZ() + localZ + 0.5 - z) /
                    horizontalRadius;
                if (nx * nx + nz * nz >= 1.0)
                    continue;
                bool touchedGrass = false;
                for (int blockY = maximumY; blockY >= minimumY; --blockY)
                {
                    const double ny = (blockY - 0.5 - y) / verticalRadius;
                    if ((nx * nx + nz * nz) + ny * ny / 6.0 >= 1.0)
                        continue;
                    const BlockType current = targetChunk.getBlock(
                        localX, blockY, localZ);
                    touchedGrass = touchedGrass || current == BlockType::Grass;
                    if (!canCarve(current))
                        continue;
                    targetChunk.setBlock(
                        localX, blockY, localZ,
                        blockY < 10 ? BlockType::Lava : BlockType::Air);
                    if (touchedGrass && blockY > 0 &&
                        targetChunk.getBlock(localX, blockY - 1, localZ) ==
                            BlockType::Dirt)
                        targetChunk.setBlock(
                            localX, blockY - 1, localZ, BlockType::Grass);
                }
            }
        }
    }
}
