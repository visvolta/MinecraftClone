#include "worldgen/LakeGenerator.h"

#include "worldgen/JavaRandom.h"
#include "worldgen/WorldGenerationContext.h"

#include <array>

LakeGenerator::LakeGenerator(BlockType liquid)
    : liquid_(liquid)
{
}

bool LakeGenerator::generate(
    WorldGenerationContext& context,
    JavaRandom& random,
    int worldX,
    int worldY,
    int worldZ) const
{
    worldX -= 8;
    worldZ -= 8;

    while (worldY > 0 &&
           isAir(context.getBlock(worldX, worldY, worldZ)))
    {
        --worldY;
    }

    worldY -= 4;
    if (worldY <= 0)
    {
        return false;
    }

    constexpr int sizeX = 16;
    constexpr int sizeZ = 16;
    constexpr int sizeY = 8;
    std::array<bool, sizeX * sizeZ * sizeY> cavity{};

    const auto index = [](int x, int z, int y)
    {
        return (x * sizeZ + z) * sizeY + y;
    };

    const int ellipsoidCount = random.nextInt(4) + 4;

    for (int ellipsoid = 0; ellipsoid < ellipsoidCount; ++ellipsoid)
    {
        const double diameterX = random.nextDouble() * 6.0 + 3.0;
        const double diameterY = random.nextDouble() * 4.0 + 2.0;
        const double diameterZ = random.nextDouble() * 6.0 + 3.0;

        const double centreX =
            random.nextDouble() * (16.0 - diameterX - 2.0) +
            1.0 +
            diameterX / 2.0;
        const double centreY =
            random.nextDouble() * (8.0 - diameterY - 4.0) +
            2.0 +
            diameterY / 2.0;
        const double centreZ =
            random.nextDouble() * (16.0 - diameterZ - 2.0) +
            1.0 +
            diameterZ / 2.0;

        for (int x = 1; x < 15; ++x)
        {
            for (int z = 1; z < 15; ++z)
            {
                for (int y = 1; y < 7; ++y)
                {
                    const double nx =
                        (static_cast<double>(x) - centreX) /
                        (diameterX / 2.0);
                    const double ny =
                        (static_cast<double>(y) - centreY) /
                        (diameterY / 2.0);
                    const double nz =
                        (static_cast<double>(z) - centreZ) /
                        (diameterZ / 2.0);

                    if (nx * nx + ny * ny + nz * nz < 1.0)
                    {
                        cavity[static_cast<std::size_t>(index(x, z, y))] =
                            true;
                    }
                }
            }
        }
    }

    for (int x = 0; x < sizeX; ++x)
    {
        for (int z = 0; z < sizeZ; ++z)
        {
            for (int y = 0; y < sizeY; ++y)
            {
                const bool boundary =
                    !cavity[static_cast<std::size_t>(index(x, z, y))] &&
                    ((x < 15 &&
                      cavity[static_cast<std::size_t>(
                          index(x + 1, z, y))]) ||
                     (x > 0 &&
                      cavity[static_cast<std::size_t>(
                          index(x - 1, z, y))]) ||
                     (z < 15 &&
                      cavity[static_cast<std::size_t>(
                          index(x, z + 1, y))]) ||
                     (z > 0 &&
                      cavity[static_cast<std::size_t>(
                          index(x, z - 1, y))]) ||
                     (y < 7 &&
                      cavity[static_cast<std::size_t>(
                          index(x, z, y + 1))]) ||
                     (y > 0 &&
                      cavity[static_cast<std::size_t>(
                          index(x, z, y - 1))]));

                if (!boundary)
                {
                    continue;
                }

                const BlockType existing =
                    context.getBlock(worldX + x, worldY + y, worldZ + z);

                if (y >= 4 && isLiquid(existing))
                {
                    return false;
                }

                if (y < 4 &&
                    !isSolid(existing) &&
                    existing != liquid_)
                {
                    return false;
                }
            }
        }
    }

    for (int x = 0; x < sizeX; ++x)
    {
        for (int z = 0; z < sizeZ; ++z)
        {
            for (int y = 0; y < sizeY; ++y)
            {
                if (!cavity[static_cast<std::size_t>(index(x, z, y))])
                {
                    continue;
                }

                context.setBlock(
                    worldX + x,
                    worldY + y,
                    worldZ + z,
                    y >= 4 ? BlockType::Air : liquid_
                );
            }
        }
    }

    // Beta restores grass beneath the upper half of a water lake when exposed
    // to sky. Without a light map yet, air above is the closest equivalent.
    for (int x = 0; x < sizeX; ++x)
    {
        for (int z = 0; z < sizeZ; ++z)
        {
            for (int y = 4; y < sizeY; ++y)
            {
                if (!cavity[static_cast<std::size_t>(index(x, z, y))])
                {
                    continue;
                }

                const int blockX = worldX + x;
                const int blockY = worldY + y - 1;
                const int blockZ = worldZ + z;

                if (context.getBlock(blockX, blockY, blockZ) ==
                        BlockType::Dirt &&
                    isAir(context.getBlock(blockX, blockY + 1, blockZ)))
                {
                    context.setBlock(
                        blockX,
                        blockY,
                        blockZ,
                        BlockType::Grass
                    );
                }
            }
        }
    }

    return true;
}
