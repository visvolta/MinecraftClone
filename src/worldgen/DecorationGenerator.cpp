#include "worldgen/DecorationGenerator.h"

#include "Chunk.h"
#include "worldgen/JavaRandom.h"
#include "worldgen/WorldGenerationContext.h"

#include <algorithm>

bool DecorationGenerator::canFlowerStay(
    const WorldGenerationContext& context,
    int x,
    int y,
    int z)
{
    if (y <= 0 || y >= Chunk::HEIGHT)
        return false;

    if (context.getBlock(x, y, z) != BlockType::Air)
        return false;

    const BlockType below = context.getBlock(x, y - 1, z);
    return below == BlockType::Grass || below == BlockType::Dirt;
}

bool DecorationGenerator::canMushroomStay(
    const WorldGenerationContext& context,
    int x,
    int y,
    int z)
{
    if (y <= 0 || y >= Chunk::HEIGHT)
        return false;

    if (context.getBlock(x, y, z) != BlockType::Air)
        return false;

    const BlockType below = context.getBlock(x, y - 1, z);
    if (!isOpaque(below))
        return false;

    // Beta mushrooms require full block light below 13. Population runs before
    // runtime lighting in this clone, so covered terrain is the stable
    // equivalent: the first uncovered block must be above the mushroom.
    return context.getHeightValue(x, z) > y + 1;
}

int DecorationGenerator::descendToGround(
    const WorldGenerationContext& context,
    int x,
    int y,
    int z)
{
    y = std::clamp(y, 0, Chunk::HEIGHT - 1);

    while (y > 0)
    {
        const BlockType block = context.getBlock(x, y, z);
        if (block != BlockType::Air && !isLeaf(block))
            break;
        --y;
    }

    return y;
}

void DecorationGenerator::generateFlowers(
    WorldGenerationContext& context,
    JavaRandom& random,
    BlockType type,
    int x,
    int y,
    int z) const
{
    // WorldGenFlowers: 64 scatter attempts.
    for (int attempt = 0; attempt < 64; ++attempt)
    {
        const int px = x + random.nextInt(8) - random.nextInt(8);
        const int py = y + random.nextInt(4) - random.nextInt(4);
        const int pz = z + random.nextInt(8) - random.nextInt(8);

        const bool mushroom =
            type == BlockType::BrownMushroom ||
            type == BlockType::RedMushroom;

        if (mushroom
                ? canMushroomStay(context, px, py, pz)
                : canFlowerStay(context, px, py, pz))
        {
            context.setBlock(px, py, pz, type);
        }
    }
}

void DecorationGenerator::generateTallGrass(
    WorldGenerationContext& context,
    JavaRandom& random,
    int x,
    int y,
    int z) const
{
    // WorldGenTallGrass first walks down through air/leaves, then performs
    // 128 scatter attempts—twice the flower generator's count.
    y = descendToGround(context, x, y, z);

    for (int attempt = 0; attempt < 128; ++attempt)
    {
        const int px = x + random.nextInt(8) - random.nextInt(8);
        const int py = y + random.nextInt(4) - random.nextInt(4);
        const int pz = z + random.nextInt(8) - random.nextInt(8);

        if (canFlowerStay(context, px, py, pz))
            context.setBlock(px, py, pz, BlockType::TallGrass);
    }
}

void DecorationGenerator::generatePumpkins(
    WorldGenerationContext& context,
    JavaRandom& random,
    int x,
    int y,
    int z) const
{
    for (int attempt = 0; attempt < 64; ++attempt)
    {
        const int px = x + random.nextInt(8) - random.nextInt(8);
        const int py = y + random.nextInt(4) - random.nextInt(4);
        const int pz = z + random.nextInt(8) - random.nextInt(8);

        if (py > 0 &&
            context.getBlock(px, py, pz) == BlockType::Air &&
            context.getBlock(px, py - 1, pz) == BlockType::Grass)
        {
            context.setBlock(px, py, pz, BlockType::Pumpkin);
        }
    }
}
