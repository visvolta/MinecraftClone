#pragma once

#include "Block.h"
#include "worldgen/Biome.h"

class JavaRandom;
class WorldGenerationContext;

class TreeGenerator
{
public:
    bool generateForBiome(
        WorldGenerationContext& context,
        JavaRandom& random,
        BiomeId biome,
        int worldX,
        int worldY,
        int worldZ
    ) const;

private:
    bool generateOak(
        WorldGenerationContext& context,
        JavaRandom& random,
        int x,
        int y,
        int z
    ) const;

    bool generateBirch(
        WorldGenerationContext& context,
        JavaRandom& random,
        int x,
        int y,
        int z
    ) const;

    bool generateSpruce(
        WorldGenerationContext& context,
        JavaRandom& random,
        int x,
        int y,
        int z
    ) const;

    bool generateJungle(
        WorldGenerationContext& context,
        JavaRandom& random,
        int x, int y, int z
    ) const;

    bool generateAcacia(
        WorldGenerationContext& context,
        JavaRandom& random,
        int x, int y, int z
    ) const;

    bool generateDarkOak(
        WorldGenerationContext& context,
        JavaRandom& random,
        int x, int y, int z
    ) const;

    bool generateMegaSpruce(
        WorldGenerationContext& context,
        JavaRandom& random,
        int x, int y, int z
    ) const;

    static bool canReplace(BlockType block) noexcept;
    static bool canOccupy(
        const WorldGenerationContext& context,
        int x,
        int y,
        int z
    ) noexcept;
    static bool hasValidSoil(
        const WorldGenerationContext& context,
        int x,
        int y,
        int z
    ) noexcept;
};
