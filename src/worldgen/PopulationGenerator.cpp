#include "worldgen/PopulationGenerator.h"

#include "Chunk.h"
#include "worldgen/ClayGenerator.h"
#include "worldgen/JavaRandom.h"
#include "worldgen/LakeGenerator.h"
#include "worldgen/OreGenerator.h"
#include "worldgen/TreeGenerator.h"
#include "worldgen/DecorationGenerator.h"
#include "worldgen/DungeonGenerator.h"
#include "worldgen/Biome.h"
#include "worldgen/WorldGenerationContext.h"

#include <bit>
#include <algorithm>
#include <cstdint>

namespace
{
std::int64_t makeOdd(std::int64_t value)
{
    return value / 2LL * 2LL + 1LL;
}

std::int64_t populationSeed(
    int chunkX,
    std::int64_t xMultiplier,
    int chunkZ,
    std::int64_t zMultiplier,
    std::int64_t worldSeed)
{
    std::uint64_t value =
        static_cast<std::uint64_t>(
            static_cast<std::int64_t>(chunkX)) *
        static_cast<std::uint64_t>(xMultiplier);

    value +=
        static_cast<std::uint64_t>(
            static_cast<std::int64_t>(chunkZ)) *
        static_cast<std::uint64_t>(zMultiplier);

    value ^= static_cast<std::uint64_t>(worldSeed);
    return std::bit_cast<std::int64_t>(value);
}
}

PopulationGenerator::PopulationGenerator(std::int64_t worldSeed)
    : worldSeed_(worldSeed)
{
}

void PopulationGenerator::populate(
    Chunk& targetChunk,
    WorldGenerationContext& context) const
{
    JavaRandom seedRandom(worldSeed_);
    const std::int64_t xMultiplier =
        makeOdd(seedRandom.nextLong());
    const std::int64_t zMultiplier =
        makeOdd(seedRandom.nextLong());

    const LakeGenerator waterLake(BlockType::Water);
    const ClayGenerator clay(4);
    const OreGenerator ores;
    const TreeGenerator trees;
    const DecorationGenerator decorations;
    const DungeonGenerator dungeons;
    const LakeGenerator lavaLake(BlockType::Lava);
    const auto generateTree = [&context, &trees](
        JavaRandom& random,
        BiomeId biome,
        int x,
        int y,
        int z)
    {
        context.beginIsolatedFeature();
        const bool generated = trees.generateForBiome(
            context, random, biome, x, y, z
        );
        context.finishIsolatedFeature(generated);
        return generated;
    };

    // Biome decoration starts features at +8 inside each source chunk, so a
    // target chunk can receive features from itself and its negative-X/Z
    // neighbours. Replaying those four origin chunks keeps borders continuous
    // while allowing each worker job to publish one immutable target chunk.
    for (int sourceChunkX =
             targetChunk.getChunkX() - 1;
         sourceChunkX <= targetChunk.getChunkX();
         ++sourceChunkX)
    {
        for (int sourceChunkZ =
                 targetChunk.getChunkZ() - 1;
             sourceChunkZ <= targetChunk.getChunkZ();
             ++sourceChunkZ)
        {
            JavaRandom random(populationSeed(
                sourceChunkX,
                xMultiplier,
                sourceChunkZ,
                zMultiplier,
                worldSeed_
            ));

            const int originX = sourceChunkX * Chunk::WIDTH;
            const int originZ = sourceChunkZ * Chunk::DEPTH;

            if (random.nextInt(4) == 0)
            {
                waterLake.generate(
                    context,
                    random,
                    originX + random.nextInt(16) + 8,
                    random.nextInt(Chunk::HEIGHT),
                    originZ + random.nextInt(16) + 8
                );
            }

            if (random.nextInt(8) == 0)
            {
                const int lakeY = random.nextInt(random.nextInt(248) + 8);
                if (lakeY < 63 || random.nextInt(10) == 0)
                    lavaLake.generate(context, random,
                        originX + random.nextInt(16) + 8,
                        lakeY,
                        originZ + random.nextInt(16) + 8);
            }

            for (int attempt=0; attempt<8; ++attempt)
                dungeons.generate(context, random,
                    originX + random.nextInt(16) + 8,
                    random.nextInt(Chunk::HEIGHT),
                    originZ + random.nextInt(16) + 8);

            ores.generate(
                context,
                random,
                originX,
                originZ
            );

            const int biomeSampleX = originX + 16;
            const int biomeSampleZ = originZ + 16;
            const ClimateSample climate =
                context.sampleClimate(biomeSampleX, biomeSampleZ);

            const BiomeDefinition* biomeDefinition =
                BiomeRegistry::active().find(climate.biome);

            const int sandPatches = biomeDefinition == nullptr
                ? 3 : biomeDefinition->sandPatchesPerChunk;
            for (int attempt = 0; attempt < sandPatches; ++attempt)
            {
                const int x = originX + random.nextInt(16) + 8;
                const int z = originZ + random.nextInt(16) + 8;
                decorations.generateSurfacePatch(
                    context, random, BlockType::Sand,
                    x, context.getHeightValue(x, z) - 1, z, 7
                );
            }
            const int clayPatches = biomeDefinition == nullptr
                ? 1 : biomeDefinition->clayPatchesPerChunk;
            for (int attempt = 0; attempt < clayPatches; ++attempt)
            {
                const int x = originX + random.nextInt(16) + 8;
                const int z = originZ + random.nextInt(16) + 8;
                clay.generate(
                    context, random, x,
                    context.getHeightValue(x, z) - 1, z
                );
            }
            const int gravelPatches = biomeDefinition == nullptr
                ? 1 : biomeDefinition->gravelPatchesPerChunk;
            for (int attempt = 0; attempt < gravelPatches; ++attempt)
            {
                const int x = originX + random.nextInt(16) + 8;
                const int z = originZ + random.nextInt(16) + 8;
                decorations.generateSurfacePatch(
                    context, random, BlockType::Gravel,
                    x, context.getHeightValue(x, z) - 1, z, 6
                );
            }
            if (biomeDefinition != nullptr &&
                biomeDefinition->roofedForestDecoration)
            {
                for (int gridX = 0; gridX < 4; ++gridX)
                {
                    for (int gridZ = 0; gridZ < 4; ++gridZ)
                    {
                        const int treeX = originX + gridX * 4 + 9 +
                            random.nextInt(3);
                        const int treeZ = originZ + gridZ * 4 + 9 +
                            random.nextInt(3);
                        if (random.nextInt(20) == 0)
                        {
                            context.beginIsolatedFeature();
                            const bool generated = decorations.generateBigMushroom(
                                context, random, treeX,
                                context.getHeightValue(treeX, treeZ), treeZ
                            );
                            context.finishIsolatedFeature(generated);
                            continue;
                        }
                        generateTree(
                            random, climate.biome,
                            treeX,
                            context.getHeightValue(treeX, treeZ),
                            treeZ
                        );
                    }
                }
            }
            else
            {
                int treeCount = biomeDefinition == nullptr
                    ? 0
                    : biomeDefinition->treesPerChunk;
                if (random.nextFloat() < (biomeDefinition == nullptr
                        ? 0.1f
                        : biomeDefinition->extraTreeChance))
                    ++treeCount;

                for (int attempt = 0; attempt < treeCount; ++attempt)
                {
                    const int treeX = originX + random.nextInt(16) + 8;
                    const int treeZ = originZ + random.nextInt(16) + 8;
                    const int treeY = context.getHeightValue(treeX, treeZ);

                    const bool generated = generateTree(
                        random, climate.biome, treeX, treeY, treeZ
                    );
                    if (generated &&
                        (climate.biome == VanillaBiomes::Jungle ||
                         climate.biome == VanillaBiomes::JungleHills ||
                         climate.biome == VanillaBiomes::JungleMountains))
                    {
                        decorations.generateCocoa(
                            context, random, treeX, treeY, treeZ
                        );
                    }
                }
            }

            const int bigMushroomCount = biomeDefinition == nullptr
                ? 0 : biomeDefinition->bigMushroomsPerChunk;
            for (int attempt = 0; attempt < bigMushroomCount; ++attempt)
            {
                const int x = originX + random.nextInt(16) + 8;
                const int z = originZ + random.nextInt(16) + 8;
                context.beginIsolatedFeature();
                const bool generated = decorations.generateBigMushroom(
                    context, random, x, context.getHeightValue(x, z), z
                );
                context.finishIsolatedFeature(generated);
            }

            const int flowerCount = biomeDefinition == nullptr
                ? 0
                : biomeDefinition->flowersPerChunk;
            for (int i = 0; i < flowerCount; ++i)
            {
                const int x = originX + random.nextInt(16) + 8;
                const int z = originZ + random.nextInt(16) + 8;
                const int maximumY = std::min(
                    Chunk::HEIGHT,
                    context.getHeightValue(x, z) + 32
                );
                decorations.generateFlowers(
                    context, random,
                    random.nextInt(3) == 0 ? BlockType::Rose
                                           : BlockType::Dandelion,
                    x, maximumY > 0 ? random.nextInt(maximumY) : 0, z
                );
            }

            const int grassCount = biomeDefinition == nullptr
                ? 0
                : biomeDefinition->grassPerChunk;
            for (int i = 0; i < grassCount; ++i)
            {
                const int x = originX + random.nextInt(16) + 8;
                const int z = originZ + random.nextInt(16) + 8;
                const int maximumY = std::min(
                    Chunk::HEIGHT,
                    context.getHeightValue(x, z) * 2
                );
                BlockType grass = BlockType::TallGrass;
                if ((climate.biome == VanillaBiomes::Taiga ||
                     climate.biome == VanillaBiomes::TaigaHills ||
                     climate.biome == VanillaBiomes::ColdTaiga ||
                     climate.biome == VanillaBiomes::ColdTaigaHills) &&
                    random.nextInt(5) != 0)
                    grass = BlockType::Fern;
                else if ((climate.biome == VanillaBiomes::Jungle ||
                          climate.biome == VanillaBiomes::JungleHills) &&
                         random.nextInt(4) == 0)
                    grass = BlockType::Fern;
                decorations.generateTallGrass(
                    context, random, grass, x,
                    maximumY > 0 ? random.nextInt(maximumY) : 0, z
                );
            }

            const int deadBushes = biomeDefinition == nullptr
                ? 0 : biomeDefinition->deadBushesPerChunk;
            for (int i = 0; i < deadBushes; ++i)
            {
                const int x = originX + random.nextInt(16) + 8;
                const int z = originZ + random.nextInt(16) + 8;
                decorations.generateFlowers(
                    context, random, BlockType::DeadBush,
                    x, random.nextInt(std::max(1,
                        std::min(Chunk::HEIGHT,
                            context.getHeightValue(x, z) * 2))), z
                );
            }

            const int mushroomCount = biomeDefinition == nullptr
                ? 0 : biomeDefinition->mushroomsPerChunk;
            for (int i = 0; i < mushroomCount; ++i)
            {
                if (random.nextInt(4) == 0)
                {
                    const int x = originX + random.nextInt(16) + 8;
                    const int z = originZ + random.nextInt(16) + 8;
                    decorations.generateFlowers(context, random,
                        BlockType::BrownMushroom, x,
                        context.getHeightValue(x, z), z);
                }
                if (random.nextInt(8) == 0)
                    decorations.generateFlowers(context,random,BlockType::RedMushroom,
                        originX+random.nextInt(16)+8,random.nextInt(Chunk::HEIGHT),
                        originZ+random.nextInt(16)+8);
            }
            if(random.nextInt(4)==0)
                decorations.generateFlowers(context,random,BlockType::BrownMushroom,
                    originX+random.nextInt(16)+8,random.nextInt(Chunk::HEIGHT),
                    originZ+random.nextInt(16)+8);
            if(random.nextInt(8)==0)
                decorations.generateFlowers(context,random,BlockType::RedMushroom,
                    originX+random.nextInt(16)+8,random.nextInt(Chunk::HEIGHT),
                    originZ+random.nextInt(16)+8);
            if(random.nextInt(32)==0)
                decorations.generatePumpkins(context,random,
                    originX+random.nextInt(16)+8,random.nextInt(Chunk::HEIGHT),
                    originZ+random.nextInt(16)+8);

            const int reedCount = (biomeDefinition == nullptr
                ? 0 : biomeDefinition->reedsPerChunk) + 10;
            for (int i = 0; i < reedCount; ++i)
            {
                const int x = originX + random.nextInt(16) + 8;
                const int z = originZ + random.nextInt(16) + 8;
                decorations.generateReeds(
                    context, random, x,
                    std::min(Chunk::HEIGHT - 1,
                        context.getHeightValue(x, z) * 2), z
                );
            }
            const int cactusCount = biomeDefinition == nullptr
                ? 0 : biomeDefinition->cactiPerChunk;
            for (int i = 0; i < cactusCount; ++i)
            {
                const int x = originX + random.nextInt(16) + 8;
                const int z = originZ + random.nextInt(16) + 8;
                decorations.generateCactus(
                    context, random, x,
                    std::min(Chunk::HEIGHT - 1,
                        context.getHeightValue(x, z) * 2), z
                );
            }
            const int melonCount = biomeDefinition == nullptr
                ? 0 : biomeDefinition->melonsPerChunk;
            for (int i = 0; i < melonCount; ++i)
            {
                const int x = originX + random.nextInt(16) + 8;
                const int z = originZ + random.nextInt(16) + 8;
                decorations.generateMelons(
                    context, random, x, random.nextInt(std::max(1,
                        std::min(Chunk::HEIGHT,
                            context.getHeightValue(x, z) * 2))), z
                );
            }
            const int vineCount = biomeDefinition == nullptr
                ? 0 : biomeDefinition->vinesPerChunk;
            for (int i = 0; i < vineCount; ++i)
                decorations.generateVines(
                    context, random,
                    originX + random.nextInt(16) + 8,
                    128,
                    originZ + random.nextInt(16) + 8
                );
        }
    }

    decorations.freezeAndSnow(
        context,
        targetChunk.getWorldOriginX(),
        targetChunk.getWorldOriginZ(),
        Chunk::WIDTH,
        Chunk::DEPTH
    );
}
