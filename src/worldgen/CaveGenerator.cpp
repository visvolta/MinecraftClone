#include "worldgen/CaveGenerator.h"

#include "Block.h"
#include "Chunk.h"
#include "worldgen/Biome.h"
#include "worldgen/JavaRandom.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <numbers>

namespace
{
std::int64_t mapGenSeed(int sourceChunkX, std::int64_t xMultiplier,
                        int sourceChunkZ, std::int64_t zMultiplier,
                        std::int64_t worldSeed) noexcept
{
    const std::uint64_t x =
        static_cast<std::uint64_t>(static_cast<std::int64_t>(sourceChunkX)) *
        static_cast<std::uint64_t>(xMultiplier);
    const std::uint64_t z =
        static_cast<std::uint64_t>(static_cast<std::int64_t>(sourceChunkZ)) *
        static_cast<std::uint64_t>(zMultiplier);
    return std::bit_cast<std::int64_t>(
        x ^ z ^ static_cast<std::uint64_t>(worldSeed));
}

bool canReplaceCaveBlock(BlockType current, BlockType above) noexcept
{
    if (current == BlockType::Stone || current == BlockType::Dirt ||
        current == BlockType::Grass || current == BlockType::Sandstone ||
        current == BlockType::Mycelium || current == BlockType::Snow)
        return true;
    return (current == BlockType::Sand || current == BlockType::Gravel) &&
           above != BlockType::Water;
}
}

CaveGenerator::CaveGenerator(std::int64_t worldSeed, int range)
    : worldSeed_(worldSeed), range_(std::max(1, range)) {}

void CaveGenerator::generate(Chunk& targetChunk) const
{
    // MapGenBase uses the two raw nextLong values. They are deliberately NOT
    // forced odd (that rule belongs to ChunkGeneratorOverworld::populate).
    JavaRandom seedRandom(worldSeed_);
    const std::int64_t xMultiplier = seedRandom.nextLong();
    const std::int64_t zMultiplier = seedRandom.nextLong();

    for (int sourceX = targetChunk.getChunkX() - range_;
         sourceX <= targetChunk.getChunkX() + range_; ++sourceX)
    {
        for (int sourceZ = targetChunk.getChunkZ() - range_;
             sourceZ <= targetChunk.getChunkZ() + range_; ++sourceZ)
        {
            JavaRandom random(mapGenSeed(
                sourceX, xMultiplier, sourceZ, zMultiplier, worldSeed_));
            recursiveGenerate(random, sourceX, sourceZ, targetChunk);
        }
    }
}

void CaveGenerator::recursiveGenerate(JavaRandom& random, int sourceChunkX,
                                      int sourceChunkZ, Chunk& targetChunk) const
{
    int caveCount = random.nextInt(
        random.nextInt(random.nextInt(15) + 1) + 1);
    if (random.nextInt(7) != 0)
        caveCount = 0;

    for (int cave = 0; cave < caveCount; ++cave)
    {
        const double startX = sourceChunkX * 16.0 + random.nextInt(16);
        const double startY = random.nextInt(random.nextInt(120) + 8);
        const double startZ = sourceChunkZ * 16.0 + random.nextInt(16);
        int branches = 1;

        if (random.nextInt(4) == 0)
        {
            generateLargeCaveNode(random, targetChunk, startX, startY, startZ);
            branches += random.nextInt(4);
        }

        for (int branch = 0; branch < branches; ++branch)
        {
            const float yaw = random.nextFloat() *
                std::numbers::pi_v<float> * 2.0f;
            const float pitch = (random.nextFloat() - 0.5f) * 2.0f / 8.0f;
            float radius = random.nextFloat() * 2.0f + random.nextFloat();
            if (random.nextInt(10) == 0)
                radius *= random.nextFloat() * random.nextFloat() * 3.0f + 1.0f;

            generateCaveNode(random, targetChunk, startX, startY, startZ,
                             radius, yaw, pitch, 0, 0, 1.0);
        }
    }
}

void CaveGenerator::generateLargeCaveNode(JavaRandom& random, Chunk& targetChunk,
                                          double x, double y, double z) const
{
    generateCaveNode(random, targetChunk, x, y, z,
        1.0f + random.nextFloat() * 6.0f,
        0.0f, 0.0f, -1, -1, 0.5);
}

void CaveGenerator::generateCaveNode(JavaRandom& parentRandom, Chunk& targetChunk,
                                     double worldX, double worldY, double worldZ,
                                     float radius, float yaw, float pitch,
                                     int step, int maxSteps,
                                     double verticalScale) const
{
    const double centreX = targetChunk.getChunkX() * 16.0 + 8.0;
    const double centreZ = targetChunk.getChunkZ() * 16.0 + 8.0;
    float yawVelocity = 0.0f;
    float pitchVelocity = 0.0f;
    JavaRandom random(parentRandom.nextLong());

    if (maxSteps <= 0)
    {
        const int distance = range_ * 16 - 16;
        maxSteps = distance - random.nextInt(distance / 4);
    }

    bool room = false;
    if (step == -1)
    {
        step = maxSteps / 2;
        room = true;
    }

    const int branchStep = random.nextInt(maxSteps / 2) + maxSteps / 4;
    const bool gentlePitch = random.nextInt(6) == 0;

    for (; step < maxSteps; ++step)
    {
        const double horizontalRadius = 1.5 +
            std::sin(static_cast<float>(step) * std::numbers::pi_v<float> /
                     static_cast<float>(maxSteps)) * radius;
        const double verticalRadius = horizontalRadius * verticalScale;
        const float cosPitch = std::cos(pitch);
        const float sinPitch = std::sin(pitch);
        worldX += std::cos(yaw) * cosPitch;
        worldY += sinPitch;
        worldZ += std::sin(yaw) * cosPitch;

        pitch *= gentlePitch ? 0.92f : 0.7f;
        pitch += pitchVelocity * 0.1f;
        yaw += yawVelocity * 0.1f;
        pitchVelocity *= 0.9f;
        yawVelocity *= 0.75f;
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

        if (!room && step == branchStep && radius > 1.0f && maxSteps > 0)
        {
            generateCaveNode(random, targetChunk, worldX, worldY, worldZ,
                random.nextFloat() * 0.5f + 0.5f,
                yaw - std::numbers::pi_v<float> / 2.0f,
                pitch / 3.0f, step, maxSteps, 1.0);
            generateCaveNode(random, targetChunk, worldX, worldY, worldZ,
                random.nextFloat() * 0.5f + 0.5f,
                yaw + std::numbers::pi_v<float> / 2.0f,
                pitch / 3.0f, step, maxSteps, 1.0);
            return;
        }

        if (!room && random.nextInt(4) == 0)
            continue;

        const double dx = worldX - centreX;
        const double dz = worldZ - centreZ;
        const double remaining = maxSteps - step;
        const double reach = radius + 2.0f + 16.0f;
        if (dx * dx + dz * dz - remaining * remaining > reach * reach)
            return;

        if (worldX < centreX - 16.0 - horizontalRadius * 2.0 ||
            worldZ < centreZ - 16.0 - horizontalRadius * 2.0 ||
            worldX > centreX + 16.0 + horizontalRadius * 2.0 ||
            worldZ > centreZ + 16.0 + horizontalRadius * 2.0)
            continue;

        int minX = static_cast<int>(std::floor(worldX - horizontalRadius)) -
                   targetChunk.getWorldOriginX() - 1;
        int maxX = static_cast<int>(std::floor(worldX + horizontalRadius)) -
                   targetChunk.getWorldOriginX() + 1;
        int minY = static_cast<int>(std::floor(worldY - verticalRadius)) - 1;
        int maxY = static_cast<int>(std::floor(worldY + verticalRadius)) + 1;
        int minZ = static_cast<int>(std::floor(worldZ - horizontalRadius)) -
                   targetChunk.getWorldOriginZ() - 1;
        int maxZ = static_cast<int>(std::floor(worldZ + horizontalRadius)) -
                   targetChunk.getWorldOriginZ() + 1;
        minX = std::max(minX, 0); maxX = std::min(maxX, 16);
        minY = std::max(minY, 1); maxY = std::min(maxY, 248);
        minZ = std::max(minZ, 0); maxZ = std::min(maxZ, 16);

        bool water = false;
        for (int x = minX; !water && x < maxX; ++x)
        for (int z = minZ; !water && z < maxZ; ++z)
        for (int y = maxY + 1; !water && y >= minY - 1; --y)
        {
            if (y >= 0 && y < 256)
            {
                const BlockType block = targetChunk.getBlock(x, y, z);
                if (block == BlockType::Water)
                    water = true;
                if (y != minY - 1 && x != minX && x != maxX - 1 &&
                    z != minZ && z != maxZ - 1)
                    y = minY;
            }
        }
        if (water)
            continue;

        for (int x = minX; x < maxX; ++x)
        {
            const double nx = (x + targetChunk.getWorldOriginX() + 0.5 - worldX) /
                              horizontalRadius;
            for (int z = minZ; z < maxZ; ++z)
            {
                const double nz = (z + targetChunk.getWorldOriginZ() + 0.5 - worldZ) /
                                  horizontalRadius;
                if (nx * nx + nz * nz >= 1.0)
                    continue;

                bool exposedSurface = false;
                // MCP loops j2=maxY down while j2>minY and computes the
                // ellipsoid Y from (j2-1)+0.5, but edits block j2.
                for (int y = maxY; y > minY; --y)
                {
                    const double ny = (static_cast<double>(y - 1) + 0.5 - worldY) /
                                      verticalRadius;
                    if (ny <= -0.7 || nx * nx + ny * ny + nz * nz >= 1.0)
                        continue;

                    const BlockType current = targetChunk.getBlock(x, y, z);
                    const BlockType above = y + 1 < 256
                        ? targetChunk.getBlock(x, y + 1, z)
                        : BlockType::Air;
                    if (current == BlockType::Grass || current == BlockType::Mycelium)
                        exposedSurface = true;

                    if (!canReplaceCaveBlock(current, above))
                        continue;

                    if (y - 1 < 10)
                        targetChunk.setBlock(x, y, z, BlockType::Lava);
                    else
                    {
                        targetChunk.setBlock(x, y, z, BlockType::Air);
                        if (exposedSurface && y > 0 &&
                            targetChunk.getBlock(x, y - 1, z) == BlockType::Dirt)
                        {
                            const BiomeDefinition* biome = BiomeRegistry::active().find(
                                targetChunk.getBiome(x, z));
                            targetChunk.setBlock(x, y - 1, z,
                                biome == nullptr ? BlockType::Grass : biome->topBlock);
                        }
                    }
                }
            }
        }

        if (room)
            break;
    }
}
