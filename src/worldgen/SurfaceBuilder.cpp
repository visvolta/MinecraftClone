#include "worldgen/SurfaceBuilder.h"

#include "Chunk.h"
#include "worldgen/JavaRandom.h"

#include <algorithm>

void SurfaceBuilder::replaceColumn(
    Chunk& chunk,
    int localX,
    int localZ,
    const ClimateSample& climate,
    double sandNoise,
    double gravelNoise,
    double stoneNoise,
    JavaRandom& random) const
{
    (void)sandNoise;
    (void)gravelNoise;

    const int surfaceDepth =
        static_cast<int>(
            stoneNoise / 3.0 +
            3.0 +
            random.nextDouble() * 0.25
        );

    int remainingDepth = -1;

    BlockType topBlock =
        biomeTopBlock(climate.biome);
    BlockType fillerBlock =
        biomeFillerBlock(climate.biome);

    for (int y = Chunk::HEIGHT - 1;
         y >= 0;
         --y)
    {
        if (y <= random.nextInt(5))
        {
            chunk.setBlock(
                localX,
                y,
                localZ,
                BlockType::Bedrock
            );

            continue;
        }

        const BlockType existing =
            chunk.getBlock(localX, y, localZ);

        if (existing == BlockType::Air)
        {
            remainingDepth = -1;
            continue;
        }

        if (existing != BlockType::Stone)
        {
            continue;
        }

        if (remainingDepth == -1)
        {
            if (surfaceDepth <= 0)
            {
                topBlock = BlockType::Air;
                fillerBlock = BlockType::Stone;
            }
            else if (y >= SEA_LEVEL - 4 &&
                     y <= SEA_LEVEL + 1)
            {
                topBlock =
                    biomeTopBlock(climate.biome);
                fillerBlock =
                    biomeFillerBlock(climate.biome);

            }

            if (y < SEA_LEVEL &&
                topBlock == BlockType::Air)
            {
                topBlock = climate.temperature < 0.15
                    ? BlockType::Ice
                    : BlockType::Water;
            }

            remainingDepth = surfaceDepth;

            if (y >= SEA_LEVEL - 1)
            {
                chunk.setBlock(localX, y, localZ, topBlock);
            }
            else if (y < SEA_LEVEL - 7 - surfaceDepth)
            {
                topBlock = BlockType::Air;
                fillerBlock = BlockType::Stone;
                chunk.setBlock(localX, y, localZ, BlockType::Gravel);
            }
            else
            {
                chunk.setBlock(localX, y, localZ, fillerBlock);
            }
        }
        else if (remainingDepth > 0)
        {
            --remainingDepth;

            chunk.setBlock(
                localX,
                y,
                localZ,
                fillerBlock
            );

            if (remainingDepth == 0 &&
                fillerBlock == BlockType::Sand &&
                surfaceDepth > 1)
            {
                remainingDepth = random.nextInt(4) + std::max(0, y - SEA_LEVEL);
                fillerBlock = BlockType::Sandstone;
            }
        }
    }
}

BlockType SurfaceBuilder::biomeTopBlock(
    BiomeId biome) noexcept
{
    const BiomeDefinition* definition = BiomeRegistry::active().find(biome);
    return definition == nullptr ? BlockType::Grass : definition->topBlock;
}

BlockType SurfaceBuilder::biomeFillerBlock(
    BiomeId biome) noexcept
{
    const BiomeDefinition* definition = BiomeRegistry::active().find(biome);
    return definition == nullptr ? BlockType::Dirt : definition->fillerBlock;
}
