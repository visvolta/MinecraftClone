#include "worldgen/OreGenerator.h"

#include "Block.h"
#include "Chunk.h"
#include "worldgen/JavaRandom.h"
#include "worldgen/MinableGenerator.h"
#include "worldgen/WorldGenerationContext.h"

namespace
{
void generateAttempts(WorldGenerationContext& context, JavaRandom& random,
                      const MinableGenerator& generator, int attempts,
                      int minimumY, int maximumY,
                      int originX, int originZ)
{
    for (int attempt = 0; attempt < attempts; ++attempt)
    {
        const int worldX = originX + random.nextInt(16);
        const int worldY = minimumY + random.nextInt(maximumY - minimumY);
        const int worldZ = originZ + random.nextInt(16);
        generator.generate(context, random, worldX, worldY, worldZ);
    }
}
}

void OreGenerator::generate(WorldGenerationContext& context, JavaRandom& random,
                            int originX, int originZ) const
{
    // ChunkGeneratorSettings defaults for 1.12.2.
    const MinableGenerator dirt(BlockType::Dirt, 33);
    const MinableGenerator gravel(BlockType::Gravel, 33);
    // The legacy BlockType enum does not expose stone variants yet. Running
    // the three generators as Stone preserves vanilla RNG consumption and
    // vein collision behavior without corrupting terrain; when variants are
    // promoted to BlockType these can map directly to granite/diorite/andesite.
    const MinableGenerator dioritePlaceholder(BlockType::Stone, 33);
    const MinableGenerator granitePlaceholder(BlockType::Stone, 33);
    const MinableGenerator andesitePlaceholder(BlockType::Stone, 33);
    const MinableGenerator coal(BlockType::CoalOre, 17);
    const MinableGenerator iron(BlockType::IronOre, 9);
    const MinableGenerator gold(BlockType::GoldOre, 9);
    const MinableGenerator redstone(BlockType::RedstoneOre, 8);
    const MinableGenerator diamond(BlockType::DiamondOre, 8);
    const MinableGenerator lapis(BlockType::LapisOre, 7);

    generateAttempts(context, random, dirt, 10, 0, 256, originX, originZ);
    generateAttempts(context, random, gravel, 8, 0, 256, originX, originZ);
    generateAttempts(context, random, dioritePlaceholder, 10, 0, 80, originX, originZ);
    generateAttempts(context, random, granitePlaceholder, 10, 0, 80, originX, originZ);
    generateAttempts(context, random, andesitePlaceholder, 10, 0, 80, originX, originZ);
    generateAttempts(context, random, coal, 20, 0, 128, originX, originZ);
    generateAttempts(context, random, iron, 20, 0, 64, originX, originZ);
    generateAttempts(context, random, gold, 2, 0, 32, originX, originZ);
    generateAttempts(context, random, redstone, 8, 0, 16, originX, originZ);
    generateAttempts(context, random, diamond, 1, 0, 16, originX, originZ);

    const int lapisX = originX + random.nextInt(16);
    const int lapisYFirst = random.nextInt(16);
    const int lapisYSecond = random.nextInt(16);
    const int lapisZ = originZ + random.nextInt(16);
    lapis.generate(context, random, lapisX, lapisYFirst + lapisYSecond, lapisZ);
}
