#include "worldgen/DecorationGenerator.h"

#include "Chunk.h"
#include "worldgen/JavaRandom.h"
#include "worldgen/WorldGenerationContext.h"

#include <algorithm>
#include <utility>

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

        const bool deadBush = type == BlockType::DeadBush;
        const bool deadBushCanStay = deadBush && py > 0 &&
            py < Chunk::HEIGHT &&
            context.getBlock(px, py, pz) == BlockType::Air &&
            (context.getBlock(px, py - 1, pz) == BlockType::Sand ||
             context.getBlock(px, py - 1, pz) == BlockType::Dirt);
        if (mushroom
                ? canMushroomStay(context, px, py, pz)
                : (deadBush ? deadBushCanStay
                            : canFlowerStay(context, px, py, pz)))
        {
            context.setBlock(px, py, pz, type);
        }
    }
}

void DecorationGenerator::generateTallGrass(
    WorldGenerationContext& context,
    JavaRandom& random,
    BlockType type,
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
            context.setBlock(px, py, pz, type);
    }
}

void DecorationGenerator::generateSurfacePatch(
    WorldGenerationContext& context,
    JavaRandom& random,
    BlockType replacement,
    int x,
    int y,
    int z,
    int maximumRadius) const
{
    if (!isLiquid(context.getBlock(x, y, z)))
        return;
    const int radius = random.nextInt(std::max(1, maximumRadius - 1)) + 2;
    for (int px = x - radius; px <= x + radius; ++px)
    {
        for (int pz = z - radius; pz <= z + radius; ++pz)
        {
            const int dx = px - x;
            const int dz = pz - z;
            if (dx * dx + dz * dz > radius * radius)
                continue;
            for (int py = y - 2; py <= y + 2; ++py)
            {
                const BlockType block = context.getBlock(px, py, pz);
                if (block == BlockType::Dirt || block == BlockType::Grass ||
                    block == BlockType::Sand || block == BlockType::Gravel)
                {
                    context.setBlock(px, py, pz, replacement);
                }
            }
        }
    }
}

void DecorationGenerator::generateReeds(
    WorldGenerationContext& context,
    JavaRandom& random,
    int x,
    int y,
    int z) const
{
    for (int attempt = 0; attempt < 20; ++attempt)
    {
        const int px = x + random.nextInt(4) - random.nextInt(4);
        const int pz = z + random.nextInt(4) - random.nextInt(4);
        int py = descendToGround(context, px, y, pz) + 1;
        if (context.getBlock(px, py, pz) != BlockType::Air)
            continue;
        const BlockType soil = context.getBlock(px, py - 1, pz);
        if (soil != BlockType::Grass && soil != BlockType::Dirt &&
            soil != BlockType::Sand)
            continue;
        const bool waterBeside =
            isLiquid(context.getBlock(px - 1, py - 1, pz)) ||
            isLiquid(context.getBlock(px + 1, py - 1, pz)) ||
            isLiquid(context.getBlock(px, py - 1, pz - 1)) ||
            isLiquid(context.getBlock(px, py - 1, pz + 1));
        if (!waterBeside)
            continue;
        const int height = 2 + random.nextInt(random.nextInt(3) + 1);
        for (int dy = 0; dy < height && py + dy < Chunk::HEIGHT; ++dy)
        {
            if (context.getBlock(px, py + dy, pz) != BlockType::Air)
                break;
            context.setBlock(px, py + dy, pz, BlockType::SugarCane);
        }
    }
}

void DecorationGenerator::generateCactus(
    WorldGenerationContext& context,
    JavaRandom& random,
    int x,
    int y,
    int z) const
{
    for (int attempt = 0; attempt < 10; ++attempt)
    {
        const int px = x + random.nextInt(8) - random.nextInt(8);
        const int pz = z + random.nextInt(8) - random.nextInt(8);
        const int py = descendToGround(context, px, y, pz) + 1;
        if (context.getBlock(px, py - 1, pz) != BlockType::Sand)
            continue;
        const int height = 1 + random.nextInt(random.nextInt(3) + 1);
        for (int dy = 0; dy < height; ++dy)
        {
            const int cy = py + dy;
            if (context.getBlock(px, cy, pz) != BlockType::Air ||
                isSolid(context.getBlock(px - 1, cy, pz)) ||
                isSolid(context.getBlock(px + 1, cy, pz)) ||
                isSolid(context.getBlock(px, cy, pz - 1)) ||
                isSolid(context.getBlock(px, cy, pz + 1)))
                break;
            context.setBlock(px, cy, pz, BlockType::Cactus);
        }
    }
}

void DecorationGenerator::generateMelons(
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
        if (py > 0 && context.getBlock(px, py, pz) == BlockType::Air &&
            context.getBlock(px, py - 1, pz) == BlockType::Grass)
            context.setBlock(px, py, pz, BlockType::Melon);
    }
}

void DecorationGenerator::generateVines(
    WorldGenerationContext& context,
    JavaRandom& random,
    int x,
    int y,
    int z) const
{
    int px = x;
    int pz = z;
    for (int py = std::min(y, Chunk::HEIGHT - 1); py > 0; --py)
    {
        if (context.getBlock(px, py, pz) != BlockType::Air)
        {
            px += random.nextInt(4) - random.nextInt(4);
            pz += random.nextInt(4) - random.nextInt(4);
            continue;
        }
        if (isSolid(context.getBlock(px - 1, py, pz)) ||
            isSolid(context.getBlock(px + 1, py, pz)) ||
            isSolid(context.getBlock(px, py, pz - 1)) ||
            isSolid(context.getBlock(px, py, pz + 1)))
        {
            context.setBlock(px, py, pz, BlockType::Vine);
        }
    }
}

void DecorationGenerator::generateCocoa(
    WorldGenerationContext& context,
    JavaRandom& random,
    int x,
    int y,
    int z) const
{
    for (int py = y; py < std::min(y + 12, Chunk::HEIGHT - 1); ++py)
    {
        if (context.getBlock(x, py, z) != BlockType::JungleLog)
            continue;
        for (const auto& [dx, dz] : {std::pair{-1, 0}, std::pair{1, 0},
                                     std::pair{0, -1}, std::pair{0, 1}})
        {
            if (random.nextInt(5) == 0 &&
                context.getBlock(x + dx, py, z + dz) == BlockType::Air)
                context.setBlock(x + dx, py, z + dz, BlockType::Cocoa);
        }
    }
}

bool DecorationGenerator::generateBigMushroom(
    WorldGenerationContext& context,
    JavaRandom& random,
    int x,
    int y,
    int z) const
{
    const int height = 4 + random.nextInt(3);
    const BlockType cap = random.nextInt(2) == 0
        ? BlockType::BrownMushroomBlock
        : BlockType::RedMushroomBlock;
    if (y <= 0 || y + height + 1 >= Chunk::HEIGHT ||
        (context.getBlock(x, y - 1, z) != BlockType::Grass &&
         context.getBlock(x, y - 1, z) != BlockType::Dirt &&
         context.getBlock(x, y - 1, z) != BlockType::Mycelium))
        return false;
    for (int py = y; py <= y + height + 1; ++py)
    {
        const int radius = py >= y + height - 1 ? 3 : 0;
        for (int px = x - radius; px <= x + radius; ++px)
            for (int pz = z - radius; pz <= z + radius; ++pz)
                if (context.getBlock(px, py, pz) != BlockType::Air &&
                    !isLeaf(context.getBlock(px, py, pz)))
                    return false;
    }
    for (int py = y; py < y + height; ++py)
        context.setBlock(x, py, z, BlockType::MushroomStem);
    const int capY = y + height;
    for (int px = x - 3; px <= x + 3; ++px)
        for (int pz = z - 3; pz <= z + 3; ++pz)
            if ((px != x - 3 && px != x + 3) ||
                (pz != z - 3 && pz != z + 3))
                context.setBlock(px, capY, pz, cap);
    return true;
}

void DecorationGenerator::freezeAndSnow(
    WorldGenerationContext& context,
    int originX,
    int originZ,
    int width,
    int depth) const
{
    for (int x = originX; x < originX + width; ++x)
    {
        for (int z = originZ; z < originZ + depth; ++z)
        {
            const ClimateSample climate = context.sampleClimate(x, z);
            if (!climate.biome || climate.temperature > 0.15)
                continue;
            const int y = context.getHeightValue(x, z);
            if (y <= 0 || y >= Chunk::HEIGHT)
                continue;
            if (context.getBlock(x, y - 1, z) == BlockType::Water)
                context.setBlock(x, y - 1, z, BlockType::Ice);
            else if (context.getBlock(x, y, z) == BlockType::Air &&
                     isSolid(context.getBlock(x, y - 1, z)))
                context.setBlock(x, y, z, BlockType::Snow);
        }
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
