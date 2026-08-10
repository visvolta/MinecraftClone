#include "worldgen/TreeGenerator.h"

#include "Chunk.h"
#include "worldgen/JavaRandom.h"
#include "worldgen/WorldGenerationContext.h"

#include <algorithm>
#include <array>
#include <cmath>

#include <glm/vec2.hpp>

bool TreeGenerator::canReplace(BlockType block) noexcept
{
    // 1.12 tree generators allow air and leaves. Existing tree blocks are
    // also accepted here so independently replayed border population cannot
    // reject one half of an overlapping deterministic tree.
    return block == BlockType::Air ||
           isLeaf(block) ||
           isLog(block);
}


bool TreeGenerator::canOccupy(
    const WorldGenerationContext& context,
    int x,
    int y,
    int z) noexcept
{
    if (y < 0 || y >= Chunk::HEIGHT)
        return false;

    // Always query the context, including outside the target chunk. The
    // fallback sampler gives every target chunk the same terrain answer, so
    // the source tree either succeeds everywhere or fails everywhere.
    return canReplace(context.getBlock(x, y, z));
}

bool TreeGenerator::hasValidSoil(
    const WorldGenerationContext& context,
    int x,
    int y,
    int z) noexcept
{
    const BlockType soil = context.getBlock(x, y - 1, z);
    return soil == BlockType::Grass || soil == BlockType::Dirt ||
           soil == BlockType::Podzol || soil == BlockType::Farmland;
}

bool TreeGenerator::generateForBiome(
    WorldGenerationContext& context,
    JavaRandom& random,
    BiomeId biome,
    int worldX,
    int worldY,
    int worldZ) const
{
    const BiomeDefinition* definition = BiomeRegistry::active().find(biome);
    const TreeFeature feature = definition == nullptr
        ? TreeFeature::Oak
        : definition->treeFeature;
    switch (feature)
    {
        case TreeFeature::None:
            return false;
        case TreeFeature::Birch:
            return generateBirch(context, random, worldX, worldY, worldZ);
        case TreeFeature::Taiga:
            return generateSpruce(context, random, worldX, worldY, worldZ);
        case TreeFeature::MegaTaiga:
            return generateMegaSpruce(context, random, worldX, worldY, worldZ);
        case TreeFeature::Jungle:
            return generateJungle(context, random, worldX, worldY, worldZ);
        case TreeFeature::Savanna:
            if (random.nextInt(5) == 0)
                return generateOak(context, random, worldX, worldY, worldZ);
            return generateAcacia(context, random, worldX, worldY, worldZ);
        case TreeFeature::RoofedForest:
            if (random.nextInt(3) == 0)
                return generateBirch(context, random, worldX, worldY, worldZ);
            return generateDarkOak(context, random, worldX, worldY, worldZ);
        case TreeFeature::Oak:
            if ((biome == VanillaBiomes::Forest ||
                 biome == VanillaBiomes::ForestHills) &&
                random.nextInt(5) == 0)
            {
                return generateBirch(
                    context, random, worldX, worldY, worldZ
                );
            }
            return generateOak(context, random, worldX, worldY, worldZ);
    }
    return false;
}

bool TreeGenerator::generateOak(
    WorldGenerationContext& context,
    JavaRandom& random,
    int x,
    int y,
    int z) const
{
    const int height = random.nextInt(3) + 4;
    if (y < 1 || y + height + 1 > Chunk::HEIGHT)
        return false;

    for (int yy = y; yy <= y + height + 1; ++yy)
    {
        int radius = yy == y ? 0 : 1;
        if (yy >= y + height - 1)
            radius = 2;

        for (int xx = x - radius; xx <= x + radius; ++xx)
            for (int zz = z - radius; zz <= z + radius; ++zz)
                if (!canOccupy(context, xx, yy, zz))
                    return false;
    }

    if (!hasValidSoil(context, x, y, z) ||
        y >= Chunk::HEIGHT - height - 1)
        return false;

    context.setBlock(x, y - 1, z, BlockType::Dirt);

    for (int yy = y - 3 + height; yy <= y + height; ++yy)
    {
        const int layer = yy - (y + height);
        const int radius = 1 - layer / 2;

        for (int xx = x - radius; xx <= x + radius; ++xx)
        {
            const int dx = xx - x;
            for (int zz = z - radius; zz <= z + radius; ++zz)
            {
                const int dz = zz - z;
                const bool corner =
                    std::abs(dx) == radius &&
                    std::abs(dz) == radius;

                if ((!corner || random.nextInt(2) == 0 || layer == 0) &&
                    canReplace(context.getBlock(xx, yy, zz)))
                {
                    context.setBlock(xx, yy, zz, BlockType::OakLeaves);
                }
            }
        }
    }

    for (int i = 0; i < height; ++i)
    {
        const BlockType current = context.getBlock(x, y + i, z);
        if (current == BlockType::Air || isLeaf(current))
            context.setBlock(x, y + i, z, BlockType::OakLog);
    }

    return true;
}

bool TreeGenerator::generateBirch(
    WorldGenerationContext& context,
    JavaRandom& random,
    int x,
    int y,
    int z) const
{
    const int height = random.nextInt(3) + 5;
    if (y < 1 || y + height + 1 > Chunk::HEIGHT)
        return false;

    for (int yy = y; yy <= y + height + 1; ++yy)
    {
        int radius = yy == y ? 0 : 1;
        if (yy >= y + height - 1)
            radius = 2;

        for (int xx = x - radius; xx <= x + radius; ++xx)
            for (int zz = z - radius; zz <= z + radius; ++zz)
                if (!canOccupy(context, xx, yy, zz))
                    return false;
    }

    if (!hasValidSoil(context, x, y, z))
        return false;

    context.setBlock(x, y - 1, z, BlockType::Dirt);

    for (int yy = y - 3 + height; yy <= y + height; ++yy)
    {
        const int layer = yy - (y + height);
        const int radius = 1 - layer / 2;

        for (int xx = x - radius; xx <= x + radius; ++xx)
        {
            const int dx = xx - x;
            for (int zz = z - radius; zz <= z + radius; ++zz)
            {
                const int dz = zz - z;
                const bool corner =
                    std::abs(dx) == radius &&
                    std::abs(dz) == radius;

                if ((!corner || random.nextInt(2) == 0 || layer == 0) &&
                    canReplace(context.getBlock(xx, yy, zz)))
                {
                    context.setBlock(xx, yy, zz, BlockType::BirchLeaves);
                }
            }
        }
    }

    for (int i = 0; i < height; ++i)
    {
        const BlockType current = context.getBlock(x, y + i, z);
        if (current == BlockType::Air || isLeaf(current))
            context.setBlock(x, y + i, z, BlockType::BirchLog);
    }

    return true;
}

bool TreeGenerator::generateSpruce(
    WorldGenerationContext& context,
    JavaRandom& random,
    int x,
    int y,
    int z) const
{
    const int height = random.nextInt(5) + 7;
    const int bareTrunk = height - random.nextInt(2) - 3;
    const int foliageHeight = height - bareTrunk;
    const int maximumRadius = 1 + random.nextInt(foliageHeight + 1);

    if (y < 1 || y + height + 1 > Chunk::HEIGHT)
        return false;

    for (int yy = y; yy <= y + height + 1; ++yy)
    {
        const int radius = yy - y < bareTrunk ? 0 : maximumRadius;
        for (int xx = x - radius; xx <= x + radius; ++xx)
            for (int zz = z - radius; zz <= z + radius; ++zz)
                if (!canOccupy(context, xx, yy, zz))
                    return false;
    }

    if (!hasValidSoil(context, x, y, z))
        return false;

    context.setBlock(x, y - 1, z, BlockType::Dirt);

    int radius = 0;
    for (int yy = y + height; yy >= y + bareTrunk; --yy)
    {
        for (int xx = x - radius; xx <= x + radius; ++xx)
        {
            const int dx = xx - x;
            for (int zz = z - radius; zz <= z + radius; ++zz)
            {
                const int dz = zz - z;
                if ((std::abs(dx) != radius ||
                     std::abs(dz) != radius ||
                     radius <= 0) &&
                    canReplace(context.getBlock(xx, yy, zz)))
                {
                    context.setBlock(xx, yy, zz, BlockType::SpruceLeaves);
                }
            }
        }

        if (radius >= 1 && yy == y + bareTrunk + 1)
            --radius;
        else if (radius < maximumRadius)
            ++radius;
    }

    for (int i = 0; i < height - 1; ++i)
    {
        const BlockType current = context.getBlock(x, y + i, z);
        if (current == BlockType::Air || isLeaf(current))
            context.setBlock(x, y + i, z, BlockType::SpruceLog);
    }

    return true;
}

bool TreeGenerator::generateJungle(
    WorldGenerationContext& context,
    JavaRandom& random,
    int x,
    int y,
    int z) const
{
    const int height = 8 + random.nextInt(5);
    if (y < 1 || y + height + 2 >= Chunk::HEIGHT ||
        !hasValidSoil(context, x, y, z))
        return false;
    for (int yy = y; yy <= y + height + 1; ++yy)
    {
        const int radius = yy >= y + height - 2 ? 2 : 0;
        for (int xx = x - radius; xx <= x + radius; ++xx)
            for (int zz = z - radius; zz <= z + radius; ++zz)
                if (!canOccupy(context, xx, yy, zz)) return false;
    }
    context.setBlock(x, y - 1, z, BlockType::Dirt);
    for (int yy = y + height - 3; yy <= y + height; ++yy)
    {
        const int radius = 1 + (y + height - yy) / 2;
        for (int xx = x - radius; xx <= x + radius; ++xx)
            for (int zz = z - radius; zz <= z + radius; ++zz)
                if ((std::abs(xx - x) != radius ||
                     std::abs(zz - z) != radius || random.nextInt(2) == 0) &&
                    canReplace(context.getBlock(xx, yy, zz)))
                    context.setBlock(xx, yy, zz, BlockType::JungleLeaves);
    }
    for (int yy = 0; yy < height; ++yy)
        if (canReplace(context.getBlock(x, y + yy, z)))
            context.setBlock(x, y + yy, z, BlockType::JungleLog);
    return true;
}

bool TreeGenerator::generateAcacia(
    WorldGenerationContext& context,
    JavaRandom& random,
    int x,
    int y,
    int z) const
{
    const int height = 5 + random.nextInt(4);
    if (y < 1 || y + height + 3 >= Chunk::HEIGHT ||
        !hasValidSoil(context, x, y, z))
        return false;
    constexpr int directions[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
    const int direction = random.nextInt(4);
    const int bendStart = height - 2 - random.nextInt(2);
    int trunkX = x;
    int trunkZ = z;
    std::array<glm::ivec2, 8> trunkPositions{};
    for (int level = 0; level < height; ++level)
    {
        if (level >= bendStart)
        {
            trunkX += directions[direction][0];
            trunkZ += directions[direction][1];
        }
        if (!canOccupy(context, trunkX, y + level, trunkZ)) return false;
        trunkPositions[static_cast<std::size_t>(level)] = {trunkX, trunkZ};
    }
    context.setBlock(x, y - 1, z, BlockType::Dirt);
    for (int level = 0; level < height; ++level)
    {
        const glm::ivec2 position = trunkPositions[static_cast<std::size_t>(level)];
        context.setBlock(position.x, y + level, position.y, BlockType::AcaciaLog);
    }
    const int canopyY = y + height;
    for (int radius = 0; radius <= 2; ++radius)
    {
        const int yy = canopyY - (radius == 2 ? 1 : 0);
        for (int xx = trunkX - radius; xx <= trunkX + radius; ++xx)
            for (int zz = trunkZ - radius; zz <= trunkZ + radius; ++zz)
                if ((radius < 2 || std::abs(xx - trunkX) != 2 ||
                     std::abs(zz - trunkZ) != 2) &&
                    canReplace(context.getBlock(xx, yy, zz)))
                    context.setBlock(xx, yy, zz, BlockType::AcaciaLeaves);
    }
    return true;
}

bool TreeGenerator::generateDarkOak(
    WorldGenerationContext& context,
    JavaRandom& random,
    int x,
    int y,
    int z) const
{
    const int height = 6 + random.nextInt(4);
    if (y < 1 || y + height + 2 >= Chunk::HEIGHT)
        return false;
    for (int dx = 0; dx < 2; ++dx)
        for (int dz = 0; dz < 2; ++dz)
            if (!hasValidSoil(context, x + dx, y, z + dz)) return false;
    for (int yy = y; yy < y + height; ++yy)
        for (int dx = 0; dx < 2; ++dx)
            for (int dz = 0; dz < 2; ++dz)
            {
                if (!canOccupy(context, x + dx, yy, z + dz)) return false;
                context.setBlock(
                    x + dx, yy, z + dz, BlockType::DarkOakLog
                );
            }
    for (int dx = 0; dx < 2; ++dx)
        for (int dz = 0; dz < 2; ++dz)
            context.setBlock(x + dx, y - 1, z + dz, BlockType::Dirt);
    for (int yy = y + height - 2; yy <= y + height + 1; ++yy)
    {
        const int radius = yy == y + height + 1 ? 1 : 3;
        for (int xx = x - radius; xx <= x + 1 + radius; ++xx)
            for (int zz = z - radius; zz <= z + 1 + radius; ++zz)
                if ((radius != 3 || std::abs(xx - x) < 3 ||
                     std::abs(zz - z) < 3) &&
                    canReplace(context.getBlock(xx, yy, zz)))
                    context.setBlock(xx, yy, zz, BlockType::DarkOakLeaves);
    }
    return true;
}

bool TreeGenerator::generateMegaSpruce(
    WorldGenerationContext& context,
    JavaRandom& random,
    int x,
    int y,
    int z) const
{
    const int height = 13 + random.nextInt(9);
    if (y < 1 || y + height + 2 >= Chunk::HEIGHT)
        return false;
    for (int dx = 0; dx < 2; ++dx)
        for (int dz = 0; dz < 2; ++dz)
            if (!hasValidSoil(context, x + dx, y, z + dz)) return false;
    for (int dx = 0; dx < 2; ++dx)
        for (int dz = 0; dz < 2; ++dz)
            context.setBlock(x + dx, y - 1, z + dz, BlockType::Podzol);
    const int foliageStart = y + height - 6;
    for (int yy = foliageStart; yy <= y + height; ++yy)
    {
        const int radius = std::min(3, 1 + (y + height - yy) / 2);
        for (int xx = x - radius; xx <= x + 1 + radius; ++xx)
            for (int zz = z - radius; zz <= z + 1 + radius; ++zz)
                if ((std::abs(xx - x) < radius + 1 ||
                     std::abs(zz - z) < radius + 1) &&
                    canReplace(context.getBlock(xx, yy, zz)))
                    context.setBlock(xx, yy, zz, BlockType::SpruceLeaves);
    }
    for (int yy = y; yy < y + height; ++yy)
        for (int dx = 0; dx < 2; ++dx)
            for (int dz = 0; dz < 2; ++dz)
                if (canReplace(context.getBlock(x + dx, yy, z + dz)))
                    context.setBlock(
                        x + dx, yy, z + dz, BlockType::SpruceLog
                    );
    return true;
}
