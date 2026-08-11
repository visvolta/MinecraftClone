#include "worldgen/OreGenerator.h"

#include "Block.h"
#include "worldgen/JavaRandom.h"
#include "worldgen/MinableGenerator.h"
#include "worldgen/Vanilla112State.h"
#include "worldgen/WorldGenerationContext.h"

#include <utility>

namespace
{
void generateAttempts(
    WorldGenerationContext& context,
    JavaRandom& random,
    const MinableGenerator& generator,
    int attempts,
    int minimumY,
    int maximumY,
    int originX,
    int originZ)
{
    // BiomeDecorator::genStandardOre1. Keep the Random call order x, y, z.
    if (maximumY < minimumY)
        std::swap(maximumY, minimumY);
    else if (maximumY == minimumY)
    {
        if (minimumY < 255) ++maximumY;
        else --minimumY;
    }

    for (int attempt = 0; attempt < attempts; ++attempt)
    {
        const int worldX = originX + random.nextInt(16);
        const int worldY = minimumY + random.nextInt(maximumY - minimumY);
        const int worldZ = originZ + random.nextInt(16);
        generator.generate(context, random, worldX, worldY, worldZ);
    }
}
}

void OreGenerator::generate(
    WorldGenerationContext& context,
    JavaRandom& random,
    int originX,
    int originZ) const
{
    // Minecraft 1.12.2 BiomeDecorator + default ChunkGeneratorSettings.
    // Stone variants are real BlockStates; none are RNG-only placeholders.
    const MinableGenerator dirt(BlockType::Dirt, 33);
    const MinableGenerator gravel(BlockType::Gravel, 33);
    const MinableGenerator diorite(mc112::state("diorite"), 33);
    const MinableGenerator granite(mc112::state("granite"), 33);
    const MinableGenerator andesite(mc112::state("andesite"), 33);
    const MinableGenerator coal(BlockType::CoalOre, 17);
    const MinableGenerator iron(BlockType::IronOre, 9);
    const MinableGenerator gold(BlockType::GoldOre, 9);
    const MinableGenerator redstone(BlockType::RedstoneOre, 8);
    const MinableGenerator diamond(BlockType::DiamondOre, 8);
    const MinableGenerator lapis(BlockType::LapisOre, 7);

    // BiomeDecorator::generateOres order is significant for seed parity.
    generateAttempts(context, random, dirt, 10, 0, 256, originX, originZ);
    generateAttempts(context, random, gravel, 8, 0, 256, originX, originZ);
    generateAttempts(context, random, diorite, 10, 0, 80, originX, originZ);
    generateAttempts(context, random, granite, 10, 0, 80, originX, originZ);
    generateAttempts(context, random, andesite, 10, 0, 80, originX, originZ);
    generateAttempts(context, random, coal, 20, 0, 128, originX, originZ);
    generateAttempts(context, random, iron, 20, 0, 64, originX, originZ);
    generateAttempts(context, random, gold, 2, 0, 32, originX, originZ);
    generateAttempts(context, random, redstone, 8, 0, 16, originX, originZ);
    generateAttempts(context, random, diamond, 1, 0, 16, originX, originZ);

    // BiomeDecorator::genStandardOre2 for lapis. Defaults are center=16,
    // spread=16, so center-spread is zero.
    const int lapisX = originX + random.nextInt(16);
    const int lapisY = random.nextInt(16) + random.nextInt(16);
    const int lapisZ = originZ + random.nextInt(16);
    lapis.generate(context, random, lapisX, lapisY, lapisZ);
}
