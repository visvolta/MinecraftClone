#include "worldgen/CaveGenerator.h"

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

std::int64_t caveSeed(
    int sourceChunkX,
    std::int64_t xMultiplier,
    int sourceChunkZ,
    std::int64_t zMultiplier,
    std::int64_t worldSeed)
{
    std::uint64_t value =
        static_cast<std::uint64_t>(
            static_cast<std::int64_t>(sourceChunkX)) *
        static_cast<std::uint64_t>(xMultiplier);

    value +=
        static_cast<std::uint64_t>(
            static_cast<std::int64_t>(sourceChunkZ)) *
        static_cast<std::uint64_t>(zMultiplier);

    value ^= static_cast<std::uint64_t>(worldSeed);
    return std::bit_cast<std::int64_t>(value);
}
}

CaveGenerator::CaveGenerator(std::int64_t worldSeed, int range)
    : worldSeed_(worldSeed),
      range_(std::max(1, range))
{
}

void CaveGenerator::generate(Chunk& targetChunk) const
{
    JavaRandom seedRandom(worldSeed_);
    const std::int64_t xMultiplier = makeOdd(seedRandom.nextLong());
    const std::int64_t zMultiplier = makeOdd(seedRandom.nextLong());

    for (int sourceChunkX =
             targetChunk.getChunkX() - range_;
         sourceChunkX <= targetChunk.getChunkX() + range_;
         ++sourceChunkX)
    {
        for (int sourceChunkZ =
                 targetChunk.getChunkZ() - range_;
             sourceChunkZ <= targetChunk.getChunkZ() + range_;
             ++sourceChunkZ)
        {
            JavaRandom random(caveSeed(
                sourceChunkX,
                xMultiplier,
                sourceChunkZ,
                zMultiplier,
                worldSeed_
            ));

            recursiveGenerate(
                random,
                sourceChunkX,
                sourceChunkZ,
                targetChunk
            );
        }
    }
}

void CaveGenerator::recursiveGenerate(
    JavaRandom& random,
    int sourceChunkX,
    int sourceChunkZ,
    Chunk& targetChunk) const
{
    int caveCount =
        random.nextInt(
            random.nextInt(
                random.nextInt(40) + 1
            ) + 1
        );

    if (random.nextInt(15) != 0)
    {
        caveCount = 0;
    }

    for (int cave = 0; cave < caveCount; ++cave)
    {
        const double startX =
            static_cast<double>(
                sourceChunkX * Chunk::WIDTH +
                random.nextInt(Chunk::WIDTH)
            );

        const double startY =
            static_cast<double>(
                random.nextInt(
                    random.nextInt(120) + 8
                )
            );

        const double startZ =
            static_cast<double>(
                sourceChunkZ * Chunk::DEPTH +
                random.nextInt(Chunk::DEPTH)
            );

        int branchCount = 1;

        if (random.nextInt(4) == 0)
        {
            generateLargeCaveNode(
                random,
                targetChunk,
                startX,
                startY,
                startZ
            );

            branchCount += random.nextInt(4);
        }

        for (int branch = 0; branch < branchCount; ++branch)
        {
            const float yaw =
                random.nextFloat() *
                std::numbers::pi_v<float> *
                2.0f;

            const float pitch =
                (random.nextFloat() - 0.5f) *
                2.0f /
                8.0f;

            const float radius =
                random.nextFloat() * 2.0f +
                random.nextFloat();

            generateCaveNode(
                random,
                targetChunk,
                startX,
                startY,
                startZ,
                radius,
                yaw,
                pitch,
                0,
                0,
                1.0
            );
        }
    }
}

void CaveGenerator::generateLargeCaveNode(
    JavaRandom& random,
    Chunk& targetChunk,
    double worldX,
    double worldY,
    double worldZ) const
{
    generateCaveNode(
        random,
        targetChunk,
        worldX,
        worldY,
        worldZ,
        1.0f + random.nextFloat() * 6.0f,
        0.0f,
        0.0f,
        -1,
        -1,
        0.5
    );
}

void CaveGenerator::generateCaveNode(
    JavaRandom& parentRandom,
    Chunk& targetChunk,
    double worldX,
    double worldY,
    double worldZ,
    float radius,
    float yaw,
    float pitch,
    int step,
    int maxSteps,
    double verticalScale) const
{
    const double targetCentreX =
        static_cast<double>(
            targetChunk.getChunkX() * Chunk::WIDTH + 8
        );
    const double targetCentreZ =
        static_cast<double>(
            targetChunk.getChunkZ() * Chunk::DEPTH + 8
        );

    float yawVelocity = 0.0f;
    float pitchVelocity = 0.0f;
    JavaRandom random(parentRandom.nextLong());

    if (maxSteps <= 0)
    {
        const int distance = range_ * 16 - 16;
        maxSteps = distance - random.nextInt(distance / 4);
    }

    bool initialRoom = false;

    if (step == -1)
    {
        step = maxSteps / 2;
        initialRoom = true;
    }

    const int branchStep =
        random.nextInt(maxSteps / 2) +
        maxSteps / 4;

    const bool gentlePitch = random.nextInt(6) == 0;

    for (; step < maxSteps; ++step)
    {
        const double horizontalRadius =
            1.5 +
            static_cast<double>(
                std::sin(
                    static_cast<float>(step) *
                    std::numbers::pi_v<float> /
                    static_cast<float>(maxSteps)
                ) *
                radius
            );

        const double verticalRadius =
            horizontalRadius * verticalScale;

        const float pitchCos = std::cos(pitch);
        const float pitchSin = std::sin(pitch);

        worldX += static_cast<double>(std::cos(yaw) * pitchCos);
        worldY += static_cast<double>(pitchSin);
        worldZ += static_cast<double>(std::sin(yaw) * pitchCos);

        pitch *= gentlePitch ? 0.92f : 0.7f;
        pitch += pitchVelocity * 0.1f;
        yaw += yawVelocity * 0.1f;
        pitchVelocity *= 0.9f;
        yawVelocity *= 0.75f;

        pitchVelocity +=
            (random.nextFloat() - random.nextFloat()) *
            random.nextFloat() *
            2.0f;

        yawVelocity +=
            (random.nextFloat() - random.nextFloat()) *
            random.nextFloat() *
            4.0f;

        if (!initialRoom &&
            step == branchStep &&
            radius > 1.0f)
        {
            generateCaveNode(
                random,
                targetChunk,
                worldX,
                worldY,
                worldZ,
                random.nextFloat() * 0.5f + 0.5f,
                yaw - std::numbers::pi_v<float> / 2.0f,
                pitch / 3.0f,
                step,
                maxSteps,
                1.0
            );

            generateCaveNode(
                random,
                targetChunk,
                worldX,
                worldY,
                worldZ,
                random.nextFloat() * 0.5f + 0.5f,
                yaw + std::numbers::pi_v<float> / 2.0f,
                pitch / 3.0f,
                step,
                maxSteps,
                1.0
            );

            return;
        }

        if (!initialRoom && random.nextInt(4) == 0)
        {
            continue;
        }

        const double deltaX = worldX - targetCentreX;
        const double deltaZ = worldZ - targetCentreZ;
        const double remaining =
            static_cast<double>(maxSteps - step);
        const double maximumReach =
            static_cast<double>(radius + 2.0f + 16.0f);

        if (deltaX * deltaX +
                deltaZ * deltaZ -
                remaining * remaining >
            maximumReach * maximumReach)
        {
            return;
        }

        if (worldX <
                targetCentreX -
                    16.0 -
                    horizontalRadius * 2.0 ||
            worldZ <
                targetCentreZ -
                    16.0 -
                    horizontalRadius * 2.0 ||
            worldX >
                targetCentreX +
                    16.0 +
                    horizontalRadius * 2.0 ||
            worldZ >
                targetCentreZ +
                    16.0 +
                    horizontalRadius * 2.0)
        {
            continue;
        }

        int minX =
            static_cast<int>(
                std::floor(worldX - horizontalRadius)
            ) -
            targetChunk.getWorldOriginX() -
            1;

        int maxX =
            static_cast<int>(
                std::floor(worldX + horizontalRadius)
            ) -
            targetChunk.getWorldOriginX() +
            1;

        int minY =
            static_cast<int>(
                std::floor(worldY - verticalRadius)
            ) -
            1;

        int maxY =
            static_cast<int>(
                std::floor(worldY + verticalRadius)
            ) +
            1;

        int minZ =
            static_cast<int>(
                std::floor(worldZ - horizontalRadius)
            ) -
            targetChunk.getWorldOriginZ() -
            1;

        int maxZ =
            static_cast<int>(
                std::floor(worldZ + horizontalRadius)
            ) -
            targetChunk.getWorldOriginZ() +
            1;

        minX = std::max(minX, 0);
        maxX = std::min(maxX, Chunk::WIDTH);
        minY = std::max(minY, 1);
        maxY = std::min(maxY, 120);
        minZ = std::max(minZ, 0);
        maxZ = std::min(maxZ, Chunk::DEPTH);

        bool intersectsWater = false;

        for (int x = minX;
             !intersectsWater && x < maxX;
             ++x)
        {
            for (int z = minZ;
                 !intersectsWater && z < maxZ;
                 ++z)
            {
                for (int y = maxY + 1;
                     !intersectsWater && y >= minY - 1;
                     --y)
                {
                    if (y >= 0 &&
                        y < Chunk::HEIGHT &&
                        isLiquid(targetChunk.getBlock(x, y, z)))
                    {
                        intersectsWater = true;
                    }

                    if (y != minY - 1 &&
                        x != minX &&
                        x != maxX - 1 &&
                        z != minZ &&
                        z != maxZ - 1)
                    {
                        y = minY;
                    }
                }
            }
        }

        if (intersectsWater)
        {
            continue;
        }

        for (int x = minX; x < maxX; ++x)
        {
            const double normalizedX =
                (static_cast<double>(
                     x + targetChunk.getWorldOriginX()) +
                 0.5 -
                 worldX) /
                horizontalRadius;

            for (int z = minZ; z < maxZ; ++z)
            {
                const double normalizedZ =
                    (static_cast<double>(
                         z + targetChunk.getWorldOriginZ()) +
                     0.5 -
                     worldZ) /
                    horizontalRadius;

                bool exposedGrass = false;

                if (normalizedX * normalizedX +
                        normalizedZ * normalizedZ >=
                    1.0)
                {
                    continue;
                }

                for (int y = maxY - 1; y >= minY; --y)
                {
                    const double normalizedY =
                        (static_cast<double>(y) + 0.5 - worldY) /
                        verticalRadius;

                    if (normalizedY <= -0.7 ||
                        normalizedX * normalizedX +
                                normalizedY * normalizedY +
                                normalizedZ * normalizedZ >=
                            1.0)
                    {
                        continue;
                    }

                    const BlockType current =
                        targetChunk.getBlock(x, y, z);

                    if (current == BlockType::Grass)
                    {
                        exposedGrass = true;
                    }

                    if (current == BlockType::Stone ||
                        current == BlockType::Dirt ||
                        current == BlockType::Grass)
                    {
                        // Beta fills y < 10 with lava. Lava is not in the
                        // current atlas, so these cavities remain air.
                        targetChunk.setBlock(
                            x,
                            y,
                            z,
                            BlockType::Air
                        );

                        if (exposedGrass &&
                            y > 0 &&
                            targetChunk.getBlock(x, y - 1, z) ==
                                BlockType::Dirt)
                        {
                            targetChunk.setBlock(
                                x,
                                y - 1,
                                z,
                                BlockType::Grass
                            );
                        }
                    }
                }
            }
        }

        if (initialRoom)
        {
            break;
        }
    }
}
