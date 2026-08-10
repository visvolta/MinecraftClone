#include "worldgen/TreeGenerator.h"

#include "Chunk.h"
#include "worldgen/JavaRandom.h"
#include "worldgen/WorldGenerationContext.h"

#include <algorithm>
#include <array>
#include <cmath>

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
            return random.nextInt(3) == 0
                ? generatePine(context, random, worldX, worldY, worldZ)
                : generateSpruce(context, random, worldX, worldY, worldZ);
        case TreeFeature::MegaTaiga:
            if (random.nextInt(3) == 0)
                return generateMegaSpruce(
                    context, random, worldX, worldY, worldZ
                );
            return random.nextInt(3) == 0
                ? generatePine(context, random, worldX, worldY, worldZ)
                : generateSpruce(context, random, worldX, worldY, worldZ);
        case TreeFeature::Jungle:
            if (random.nextInt(10) == 0)
                return generateBigOak(
                    context, random, worldX, worldY, worldZ
                );
            if (random.nextInt(2) == 0)
                return generateJungleShrub(
                    context, random, worldX, worldY, worldZ
                );
            if (biome != VanillaBiomes::JungleEdge &&
                biome != VanillaBiomes::JungleEdgeMountains &&
                random.nextInt(3) == 0)
            {
                return generateMegaJungle(
                    context, random, worldX, worldY, worldZ
                );
            }
            return generateJungle(context, random, worldX, worldY, worldZ);
        case TreeFeature::Savanna:
            if (random.nextInt(5) == 0)
                return generateOak(context, random, worldX, worldY, worldZ);
            return generateAcacia(context, random, worldX, worldY, worldZ);
        case TreeFeature::RoofedForest:
            if (random.nextInt(3) > 0)
                return generateDarkOak(
                    context, random, worldX, worldY, worldZ
                );
            if (random.nextInt(5) == 0)
                return generateBirch(context, random, worldX, worldY, worldZ);
            return random.nextInt(10) == 0
                ? generateBigOak(context, random, worldX, worldY, worldZ)
                : generateOak(context, random, worldX, worldY, worldZ);
        case TreeFeature::Hills:
            if (random.nextInt(3) > 0)
                return generateSpruce(
                    context, random, worldX, worldY, worldZ
                );
            return random.nextInt(10) == 0
                ? generateBigOak(context, random, worldX, worldY, worldZ)
                : generateOak(context, random, worldX, worldY, worldZ);
        case TreeFeature::Swamp:
            return generateSwamp(context, random, worldX, worldY, worldZ);
        case TreeFeature::Oak:
            if ((biome == VanillaBiomes::Forest ||
                 biome == VanillaBiomes::ForestHills ||
                 biome == VanillaBiomes::FlowerForest) &&
                random.nextInt(5) == 0)
            {
                return generateBirch(
                    context, random, worldX, worldY, worldZ
                );
            }
            return random.nextInt(10) == 0
                ? generateBigOak(context, random, worldX, worldY, worldZ)
                : generateOak(context, random, worldX, worldY, worldZ);
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

bool TreeGenerator::generateBigOak(
    WorldGenerationContext& context,
    JavaRandom& random,
    int x,
    int y,
    int z) const
{
    // WorldGenBigTree defaults: a 5..16-block height limit, a trunk ending at
    // 61.8% of that height, and four-block leaf clusters around branch nodes.
    const int heightLimit = 5 + random.nextInt(12);
    const int trunkHeight = std::min(
        heightLimit - 1,
        static_cast<int>(static_cast<float>(heightLimit) * 0.618f)
    );
    if (y < 1 || y + heightLimit + 1 > Chunk::HEIGHT ||
        !hasValidSoil(context, x, y, z))
        return false;

    for (int yy = y; yy <= y + heightLimit; ++yy)
    {
        const int radius = yy >= y + heightLimit - 5 ? 3 : 0;
        for (int xx = x - radius; xx <= x + radius; ++xx)
            for (int zz = z - radius; zz <= z + radius; ++zz)
                if (!canOccupy(context, xx, yy, zz))
                    return false;
    }

    context.setBlock(x, y - 1, z, BlockType::Dirt);
    const int crownBase = y + heightLimit - 4;
    for (int yy = crownBase; yy <= y + heightLimit; ++yy)
    {
        const int fromTop = y + heightLimit - yy;
        const int radius = fromTop == 0 ? 1 : (fromTop == 4 ? 2 : 3);
        for (int xx = x - radius; xx <= x + radius; ++xx)
        {
            for (int zz = z - radius; zz <= z + radius; ++zz)
            {
                const int dx = std::abs(xx - x);
                const int dz = std::abs(zz - z);
                if (dx == radius && dz == radius &&
                    (fromTop == 0 || random.nextInt(2) == 0))
                    continue;
                if (canReplace(context.getBlock(xx, yy, zz)))
                    context.setBlock(xx, yy, zz, BlockType::OakLeaves);
            }
        }
    }

    for (int level = 0; level <= trunkHeight; ++level)
        context.setBlock(x, y + level, z, BlockType::OakLog);

    // Vanilla big oaks connect leaf nodes back to the upper trunk. Four
    // deterministic side nodes preserve the characteristic spreading crown.
    constexpr std::array<std::array<int, 2>, 4> offsets{{
        {{2, 0}}, {{-2, 0}}, {{0, 2}}, {{0, -2}}
    }};
    for (const auto& offset : offsets)
    {
        const int branchY = crownBase + random.nextInt(3);
        const int branchX = x + offset[0];
        const int branchZ = z + offset[1];
        context.setBlock(
            x + offset[0] / 2,
            branchY - 1,
            z + offset[1] / 2,
            BlockType::OakLog
        );
        context.setBlock(branchX, branchY, branchZ, BlockType::OakLog);
        for (int dx = -1; dx <= 1; ++dx)
            for (int dz = -1; dz <= 1; ++dz)
                if (canReplace(context.getBlock(
                        branchX + dx, branchY + 1, branchZ + dz)))
                    context.setBlock(
                        branchX + dx, branchY + 1, branchZ + dz,
                        BlockType::OakLeaves
                    );
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

bool TreeGenerator::generatePine(
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

bool TreeGenerator::generateSpruce(
    WorldGenerationContext& context,
    JavaRandom& random,
    int x,
    int y,
    int z) const
{
    const int height = random.nextInt(4) + 6;
    const int bareTrunk = 1 + random.nextInt(2);
    const int foliageHeight = height - bareTrunk;
    const int maximumRadius = 2 + random.nextInt(2);
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
    int radius = random.nextInt(2);
    int nextMaximum = 1;
    int resetRadius = 0;
    for (int layer = 0; layer <= foliageHeight; ++layer)
    {
        const int yy = y + height - layer;
        for (int xx = x - radius; xx <= x + radius; ++xx)
        {
            for (int zz = z - radius; zz <= z + radius; ++zz)
            {
                if ((std::abs(xx - x) != radius ||
                     std::abs(zz - z) != radius || radius <= 0) &&
                    canReplace(context.getBlock(xx, yy, zz)))
                {
                    context.setBlock(xx, yy, zz, BlockType::SpruceLeaves);
                }
            }
        }
        if (radius >= nextMaximum)
        {
            radius = resetRadius;
            resetRadius = 1;
            nextMaximum = std::min(nextMaximum + 1, maximumRadius);
        }
        else
        {
            ++radius;
        }
    }

    const int trunkCut = random.nextInt(3);
    for (int level = 0; level < height - trunkCut; ++level)
    {
        if (canReplace(context.getBlock(x, y + level, z)))
            context.setBlock(x, y + level, z, BlockType::SpruceLog);
    }
    return true;
}

bool TreeGenerator::generateSwamp(
    WorldGenerationContext& context,
    JavaRandom& random,
    int x,
    int y,
    int z) const
{
    const int height = random.nextInt(4) + 5;
    while (y > 1 && context.getBlock(x, y - 1, z) == BlockType::Water)
        --y;
    if (y < 1 || y + height + 1 > Chunk::HEIGHT ||
        !hasValidSoil(context, x, y, z))
        return false;

    for (int yy = y; yy <= y + height + 1; ++yy)
    {
        int radius = yy == y ? 0 : 1;
        if (yy >= y + height - 1)
            radius = 3;
        for (int xx = x - radius; xx <= x + radius; ++xx)
        {
            for (int zz = z - radius; zz <= z + radius; ++zz)
            {
                const BlockType block = context.getBlock(xx, yy, zz);
                if (!canReplace(block) &&
                    !(block == BlockType::Water && yy == y))
                    return false;
            }
        }
    }

    context.setBlock(x, y - 1, z, BlockType::Dirt);
    for (int yy = y - 3 + height; yy <= y + height; ++yy)
    {
        const int layer = yy - (y + height);
        const int radius = 2 - layer / 2;
        for (int xx = x - radius; xx <= x + radius; ++xx)
        {
            for (int zz = z - radius; zz <= z + radius; ++zz)
            {
                const bool corner = std::abs(xx - x) == radius &&
                    std::abs(zz - z) == radius;
                if ((!corner || random.nextInt(2) == 0 || layer == 0) &&
                    canReplace(context.getBlock(xx, yy, zz)))
                    context.setBlock(xx, yy, zz, BlockType::OakLeaves);
            }
        }
    }
    for (int level = 0; level < height; ++level)
    {
        const BlockType block = context.getBlock(x, y + level, z);
        if (canReplace(block) || block == BlockType::Water)
            context.setBlock(x, y + level, z, BlockType::OakLog);
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
    const int height = 4 + random.nextInt(7);
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

bool TreeGenerator::generateJungleShrub(
    WorldGenerationContext& context,
    JavaRandom& random,
    int x,
    int y,
    int z) const
{
    if (y < 1 || y + 3 >= Chunk::HEIGHT ||
        !hasValidSoil(context, x, y, z))
        return false;

    context.setBlock(x, y - 1, z, BlockType::Dirt);
    for (int yy = y; yy <= y + 2; ++yy)
    {
        const int radius = 2 - (yy - y);
        for (int xx = x - radius; xx <= x + radius; ++xx)
        {
            for (int zz = z - radius; zz <= z + radius; ++zz)
            {
                const bool corner = std::abs(xx - x) == radius &&
                    std::abs(zz - z) == radius;
                if ((!corner || random.nextInt(2) == 0) &&
                    canReplace(context.getBlock(xx, yy, zz)))
                {
                    context.setBlock(
                        xx, yy, zz, BlockType::OakLeaves
                    );
                }
            }
        }
    }
    context.setBlock(x, y, z, BlockType::JungleLog);
    return true;
}

bool TreeGenerator::generateMegaJungle(
    WorldGenerationContext& context,
    JavaRandom& random,
    int x,
    int y,
    int z) const
{
    const int height = 10 + random.nextInt(20);
    if (y < 1 || y + height + 2 >= Chunk::HEIGHT)
        return false;
    for (int dx = 0; dx < 2; ++dx)
        for (int dz = 0; dz < 2; ++dz)
            if (!hasValidSoil(context, x + dx, y, z + dz))
                return false;

    for (int yy = y; yy <= y + height + 1; ++yy)
    {
        const int radius = yy >= y + height - 4 ? 4 : 1;
        for (int xx = x - radius; xx <= x + 1 + radius; ++xx)
            for (int zz = z - radius; zz <= z + 1 + radius; ++zz)
                if (!canOccupy(context, xx, yy, zz))
                    return false;
    }

    for (int dx = 0; dx < 2; ++dx)
        for (int dz = 0; dz < 2; ++dz)
            context.setBlock(x + dx, y - 1, z + dz, BlockType::Dirt);

    const int canopyTop = y + height;
    for (int yy = canopyTop - 4; yy <= canopyTop; ++yy)
    {
        const int distance = canopyTop - yy;
        const int radius = 2 + distance / 2;
        for (int xx = x - radius; xx <= x + 1 + radius; ++xx)
        {
            for (int zz = z - radius; zz <= z + 1 + radius; ++zz)
            {
                const int edgeX = std::max(x - xx, xx - (x + 1));
                const int edgeZ = std::max(z - zz, zz - (z + 1));
                if ((edgeX != radius || edgeZ != radius ||
                     random.nextInt(2) == 0) &&
                    canReplace(context.getBlock(xx, yy, zz)))
                {
                    context.setBlock(
                        xx, yy, zz, BlockType::JungleLeaves
                    );
                }
            }
        }
    }

    for (int level = 0; level < height; ++level)
        for (int dx = 0; dx < 2; ++dx)
            for (int dz = 0; dz < 2; ++dz)
                if (canReplace(context.getBlock(
                        x + dx, y + level, z + dz)))
                    context.setBlock(
                        x + dx, y + level, z + dz,
                        BlockType::JungleLog
                    );
    return true;
}

bool TreeGenerator::generateAcacia(
    WorldGenerationContext& context,
    JavaRandom& random,
    int x,
    int y,
    int z) const
{
    const int height = random.nextInt(3) + random.nextInt(3) + 5;
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

    constexpr int directions[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
    const int direction = random.nextInt(4);
    const int bendStart = height - random.nextInt(4) - 1;
    int bendLength = 3 - random.nextInt(3);
    int trunkX = x;
    int trunkZ = z;
    int canopyY = y;
    context.setBlock(x, y - 1, z, BlockType::Dirt);
    for (int level = 0; level < height; ++level)
    {
        if (level >= bendStart && bendLength > 0)
        {
            trunkX += directions[direction][0];
            trunkZ += directions[direction][1];
            --bendLength;
        }
        if (canReplace(context.getBlock(trunkX, y + level, trunkZ)))
        {
            context.setBlock(
                trunkX, y + level, trunkZ, BlockType::AcaciaLog
            );
            canopyY = y + level;
        }
    }

    for (int dx = -3; dx <= 3; ++dx)
    {
        for (int dz = -3; dz <= 3; ++dz)
            if ((std::abs(dx) != 3 || std::abs(dz) != 3) &&
                canReplace(context.getBlock(
                    trunkX + dx, canopyY, trunkZ + dz)))
                context.setBlock(
                    trunkX + dx, canopyY, trunkZ + dz,
                    BlockType::AcaciaLeaves
                );
    }
    for (int dx = -1; dx <= 1; ++dx)
        for (int dz = -1; dz <= 1; ++dz)
            if (canReplace(context.getBlock(
                    trunkX + dx, canopyY + 1, trunkZ + dz)))
                context.setBlock(
                    trunkX + dx, canopyY + 1, trunkZ + dz,
                    BlockType::AcaciaLeaves
                );
    constexpr int extended[4][2] = {{2,0},{-2,0},{0,2},{0,-2}};
    for (const auto& offset : extended)
        if (canReplace(context.getBlock(
                trunkX + offset[0], canopyY + 1, trunkZ + offset[1])))
            context.setBlock(
                trunkX + offset[0], canopyY + 1, trunkZ + offset[1],
                BlockType::AcaciaLeaves
            );

    int secondDirection = random.nextInt(4);
    if (secondDirection != direction)
    {
        const int branchStart = bendStart - random.nextInt(2) - 1;
        int branchLength = 1 + random.nextInt(3);
        int branchX = x;
        int branchZ = z;
        int branchY = 0;
        for (int level = branchStart; level < height && branchLength > 0;
             ++level, --branchLength)
        {
            if (level < 1)
                continue;
            branchX += directions[secondDirection][0];
            branchZ += directions[secondDirection][1];
            branchY = y + level;
            if (canReplace(context.getBlock(branchX, branchY, branchZ)))
                context.setBlock(
                    branchX, branchY, branchZ, BlockType::AcaciaLog
                );
        }
        if (branchY > 0)
        {
            for (int dx = -2; dx <= 2; ++dx)
                for (int dz = -2; dz <= 2; ++dz)
                    if ((std::abs(dx) != 2 || std::abs(dz) != 2) &&
                        canReplace(context.getBlock(
                            branchX + dx, branchY, branchZ + dz)))
                        context.setBlock(
                            branchX + dx, branchY, branchZ + dz,
                            BlockType::AcaciaLeaves
                        );
            for (int dx = -1; dx <= 1; ++dx)
                for (int dz = -1; dz <= 1; ++dz)
                    if (canReplace(context.getBlock(
                            branchX + dx, branchY + 1, branchZ + dz)))
                        context.setBlock(
                            branchX + dx, branchY + 1, branchZ + dz,
                            BlockType::AcaciaLeaves
                        );
        }
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
