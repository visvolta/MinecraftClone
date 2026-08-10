#include "worldgen/OreGenerator.h"

#include "Block.h"
#include "Chunk.h"
#include "worldgen/JavaRandom.h"
#include "worldgen/MinableGenerator.h"
#include "worldgen/WorldGenerationContext.h"

namespace
{
void generateAttempts(
    WorldGenerationContext& context,
    JavaRandom& random,
    const MinableGenerator& generator,
    int attempts,
    int maximumY,
    int sourceChunkOriginX,
    int sourceChunkOriginZ)
{
    for (int attempt = 0; attempt < attempts; ++attempt)
    {
        const int worldX =
            sourceChunkOriginX + random.nextInt(16);
        const int worldY = random.nextInt(maximumY);
        const int worldZ =
            sourceChunkOriginZ + random.nextInt(16);

        generator.generate(
            context,
            random,
            worldX,
            worldY,
            worldZ
        );
    }
}
}

void OreGenerator::generate(
    WorldGenerationContext& context,
    JavaRandom& random,
    int sourceChunkOriginX,
    int sourceChunkOriginZ) const
{
    // Default 1.12.2 BiomeDecorator ore passes. Stone variants are omitted
    // until their legacy compatibility block types are registered.
    const MinableGenerator dirt(BlockType::Dirt, 33);
    const MinableGenerator gravel(BlockType::Gravel, 33);
    const MinableGenerator coal(BlockType::CoalOre, 17);
    const MinableGenerator iron(BlockType::IronOre, 8);
    const MinableGenerator gold(BlockType::GoldOre, 8);
    const MinableGenerator redstone(BlockType::RedstoneOre, 7);
    const MinableGenerator diamond(BlockType::DiamondOre, 7);
    const MinableGenerator lapis(BlockType::LapisOre, 6);

    generateAttempts(
        context,
        random,
        dirt,
        10,
        Chunk::HEIGHT,
        sourceChunkOriginX,
        sourceChunkOriginZ
    );

    generateAttempts(
        context,
        random,
        gravel,
        8,
        Chunk::HEIGHT,
        sourceChunkOriginX,
        sourceChunkOriginZ
    );

    generateAttempts(
        context,
        random,
        coal,
        20,
        128,
        sourceChunkOriginX,
        sourceChunkOriginZ
    );

    generateAttempts(
        context,
        random,
        iron,
        20,
        64,
        sourceChunkOriginX,
        sourceChunkOriginZ
    );

    generateAttempts(
        context,
        random,
        gold,
        2,
        32,
        sourceChunkOriginX,
        sourceChunkOriginZ
    );

    generateAttempts(
        context,
        random,
        redstone,
        8,
        16,
        sourceChunkOriginX,
        sourceChunkOriginZ
    );

    generateAttempts(
        context,
        random,
        diamond,
        1,
        16,
        sourceChunkOriginX,
        sourceChunkOriginZ
    );

    // Lapis uses 1.12's triangular distribution around Y 16.
    lapis.generate(
        context,
        random,
        sourceChunkOriginX + random.nextInt(16),
        random.nextInt(16) + random.nextInt(16),
        sourceChunkOriginZ + random.nextInt(16)
    );
}
