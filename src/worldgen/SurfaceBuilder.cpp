#include "worldgen/SurfaceBuilder.h"

#include "Chunk.h"
#include "worldgen/JavaRandom.h"

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
    const bool useSand =
        sandNoise +
            random.nextDouble() * 0.2 >
        0.0;

    const bool useGravel =
        gravelNoise +
            random.nextDouble() * 0.2 >
        3.0;

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

                if (useGravel)
                {
                    topBlock = BlockType::Air;
                    fillerBlock = BlockType::Gravel;
                }

                if (useSand)
                {
                    topBlock = BlockType::Sand;
                    fillerBlock = BlockType::Sand;
                }
            }

            if (y < SEA_LEVEL &&
                topBlock == BlockType::Air)
            {
                topBlock = BlockType::Water;
            }

            remainingDepth = surfaceDepth;

            chunk.setBlock(
                localX,
                y,
                localZ,
                y >= SEA_LEVEL - 1
                    ? topBlock
                    : fillerBlock
            );
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
                fillerBlock == BlockType::Sand)
            {
                remainingDepth = random.nextInt(4);
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
