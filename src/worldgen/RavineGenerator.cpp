#include "worldgen/RavineGenerator.h"

#include "Block.h"
#include "Chunk.h"
#include "worldgen/Biome.h"
#include "worldgen/JavaRandom.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <numbers>

namespace
{
std::int64_t mapGenSeed(int chunkX, std::int64_t xMultiplier,
                        int chunkZ, std::int64_t zMultiplier,
                        std::int64_t worldSeed) noexcept
{
    const std::uint64_t x =
        static_cast<std::uint64_t>(static_cast<std::int64_t>(chunkX)) *
        static_cast<std::uint64_t>(xMultiplier);
    const std::uint64_t z =
        static_cast<std::uint64_t>(static_cast<std::int64_t>(chunkZ)) *
        static_cast<std::uint64_t>(zMultiplier);
    return std::bit_cast<std::int64_t>(
        x ^ z ^ static_cast<std::uint64_t>(worldSeed));
}
}

RavineGenerator::RavineGenerator(std::int64_t worldSeed, int range)
    : worldSeed_(worldSeed), range_(std::max(1, range)) {}

void RavineGenerator::generate(Chunk& targetChunk) const
{
    JavaRandom seedRandom(worldSeed_);
    const std::int64_t xMultiplier = seedRandom.nextLong();
    const std::int64_t zMultiplier = seedRandom.nextLong();

    for (int sourceX = targetChunk.getChunkX() - range_;
         sourceX <= targetChunk.getChunkX() + range_; ++sourceX)
    for (int sourceZ = targetChunk.getChunkZ() - range_;
         sourceZ <= targetChunk.getChunkZ() + range_; ++sourceZ)
    {
        JavaRandom random(mapGenSeed(
            sourceX, xMultiplier, sourceZ, zMultiplier, worldSeed_));
        generateFrom(random, sourceX, sourceZ, targetChunk);
    }
}

void RavineGenerator::generateFrom(JavaRandom& sourceRandom,
                                   int sourceChunkX, int sourceChunkZ,
                                   Chunk& targetChunk) const
{
    if (sourceRandom.nextInt(50) != 0)
        return;

    double x = sourceChunkX * 16.0 + sourceRandom.nextInt(16);
    double y = sourceRandom.nextInt(sourceRandom.nextInt(40) + 8) + 20.0;
    double z = sourceChunkZ * 16.0 + sourceRandom.nextInt(16);
    float yaw = sourceRandom.nextFloat() * std::numbers::pi_v<float> * 2.0f;
    float pitch = (sourceRandom.nextFloat() - 0.5f) * 2.0f / 8.0f;
    const float width = (sourceRandom.nextFloat() * 2.0f +
                         sourceRandom.nextFloat()) * 2.0f;
    JavaRandom random(sourceRandom.nextLong());

    const double targetCentreX = targetChunk.getChunkX() * 16.0 + 8.0;
    const double targetCentreZ = targetChunk.getChunkZ() * 16.0 + 8.0;
    const int baseDistance = range_ * 16 - 16;
    const int maximumSteps = baseDistance - random.nextInt(baseDistance / 4);
    float yawVelocity = 0.0f;
    float pitchVelocity = 0.0f;

    // MapGenRavine generates this vertical-width table once per tunnel. It is
    // a large part of the characteristic uneven ravine wall profile.
    std::array<float, 1024> verticalWidths{};
    float scale = 1.0f;
    for (int level = 0; level < 256; ++level)
    {
        if (level == 0 || random.nextInt(3) == 0)
        {
            const float scaleA = random.nextFloat();
            const float scaleB = random.nextFloat();
            scale = 1.0f + scaleA * scaleB;
        }
        verticalWidths[static_cast<std::size_t>(level)] = scale * scale;
    }

    for (int step = 0; step < maximumSteps; ++step)
    {
        double horizontalRadius = 1.5 +
            std::sin(static_cast<float>(step) * std::numbers::pi_v<float> /
                     static_cast<float>(maximumSteps)) * width;
        double verticalRadius = horizontalRadius * 3.0;
        horizontalRadius *= random.nextFloat() * 0.25 + 0.75;
        verticalRadius *= random.nextFloat() * 0.25 + 0.75;

        const float cosPitch = std::cos(pitch);
        x += std::cos(yaw) * cosPitch;
        y += std::sin(pitch);
        z += std::sin(yaw) * cosPitch;
        pitch *= 0.7f;
        pitch += pitchVelocity * 0.05f;
        yaw += yawVelocity * 0.05f;
        pitchVelocity *= 0.8f;
        yawVelocity *= 0.5f;
        const float pitchRandomA = random.nextFloat();
        const float pitchRandomB = random.nextFloat();
        const float pitchRandomScale = random.nextFloat();
        pitchVelocity +=
            (pitchRandomA - pitchRandomB) * pitchRandomScale * 2.0f;
        const float yawRandomA = random.nextFloat();
        const float yawRandomB = random.nextFloat();
        const float yawRandomScale = random.nextFloat();
        yawVelocity +=
            (yawRandomA - yawRandomB) * yawRandomScale * 4.0f;

        if (random.nextInt(4) == 0)
            continue;

        const double dx = x - targetCentreX;
        const double dz = z - targetCentreZ;
        const double remaining = maximumSteps - step;
        const double reach = width + 2.0f + 16.0f;
        if (dx * dx + dz * dz - remaining * remaining > reach * reach)
            return;

        if (x < targetCentreX - 16.0 - horizontalRadius * 2.0 ||
            z < targetCentreZ - 16.0 - horizontalRadius * 2.0 ||
            x > targetCentreX + 16.0 + horizontalRadius * 2.0 ||
            z > targetCentreZ + 16.0 + horizontalRadius * 2.0)
            continue;

        int minX = static_cast<int>(std::floor(x - horizontalRadius)) -
                   targetChunk.getWorldOriginX() - 1;
        int maxX = static_cast<int>(std::floor(x + horizontalRadius)) -
                   targetChunk.getWorldOriginX() + 1;
        int minY = static_cast<int>(std::floor(y - verticalRadius)) - 1;
        int maxY = static_cast<int>(std::floor(y + verticalRadius)) + 1;
        int minZ = static_cast<int>(std::floor(z - horizontalRadius)) -
                   targetChunk.getWorldOriginZ() - 1;
        int maxZ = static_cast<int>(std::floor(z + horizontalRadius)) -
                   targetChunk.getWorldOriginZ() + 1;
        minX = std::max(minX, 0); maxX = std::min(maxX, 16);
        minY = std::max(minY, 1); maxY = std::min(maxY, 248);
        minZ = std::max(minZ, 0); maxZ = std::min(maxZ, 16);

        bool water = false;
        for (int lx = minX; !water && lx < maxX; ++lx)
        for (int lz = minZ; !water && lz < maxZ; ++lz)
        for (int ly = maxY + 1; !water && ly >= minY - 1; --ly)
        {
            if (ly >= 0 && ly < 256)
            {
                if (targetChunk.getBlock(lx, ly, lz) == BlockType::Water)
                    water = true;
                if (ly != minY - 1 && lx != minX && lx != maxX - 1 &&
                    lz != minZ && lz != maxZ - 1)
                    ly = minY;
            }
        }
        if (water)
            continue;

        for (int lx = minX; lx < maxX; ++lx)
        {
            const double nx = (lx + targetChunk.getWorldOriginX() + 0.5 - x) /
                              horizontalRadius;
            for (int lz = minZ; lz < maxZ; ++lz)
            {
                const double nz = (lz + targetChunk.getWorldOriginZ() + 0.5 - z) /
                                  horizontalRadius;
                if (nx * nx + nz * nz >= 1.0)
                    continue;

                bool touchedGrass = false;
                for (int blockY = maxY; blockY > minY; --blockY)
                {
                    const double ny = (static_cast<double>(blockY - 1) + 0.5 - y) /
                                      verticalRadius;
                    if ((nx * nx + nz * nz) *
                            verticalWidths[static_cast<std::size_t>(blockY - 1)] +
                        ny * ny / 6.0 >= 1.0)
                        continue;

                    const BlockType current = targetChunk.getBlock(lx, blockY, lz);
                    if (current == BlockType::Grass)
                        touchedGrass = true;
                    // Vanilla ravines carve only these three blocks.
                    if (current != BlockType::Stone && current != BlockType::Dirt &&
                        current != BlockType::Grass)
                        continue;

                    if (blockY - 1 < 10)
                        targetChunk.setBlock(lx, blockY, lz, BlockType::Lava);
                    else
                    {
                        targetChunk.setBlock(lx, blockY, lz, BlockType::Air);
                        if (touchedGrass && blockY > 0 &&
                            targetChunk.getBlock(lx, blockY - 1, lz) == BlockType::Dirt)
                        {
                            const BiomeDefinition* biome = BiomeRegistry::active().find(
                                targetChunk.getBiome(lx, lz));
                            targetChunk.setBlock(lx, blockY - 1, lz,
                                biome == nullptr ? BlockType::Grass : biome->topBlock);
                        }
                    }
                }
            }
        }
    }
}
