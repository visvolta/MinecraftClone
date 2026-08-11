#include "worldgen/TreeGenerator.h"

#include "Chunk.h"
#include "content/BlockState.h"
#include "worldgen/JavaRandom.h"
#include "worldgen/WorldGenerationContext.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <vector>
#include <utility>

namespace
{
struct Direction
{
    int x;
    int z;
};

constexpr std::array<Direction, 4> Horizontal{{
    {0, -1}, {0, 1}, {-1, 0}, {1, 0}
}};

bool airOrLeaves(BlockType block) noexcept
{
    return block == BlockType::Air || isLeaf(block);
}

bool airLeavesOrVine(BlockType block) noexcept
{
    return block == BlockType::Air || isLeaf(block) || block == BlockType::Vine;
}

void setDirtAt(WorldGenerationContext& context, int x, int y, int z)
{
    if (context.getBlock(x, y, z) != BlockType::Dirt)
        context.setBlock(x, y, z, BlockType::Dirt);
}

void placeLeaf(
    WorldGenerationContext& context,
    int x,
    int y,
    int z,
    BlockType leaves,
    bool requireAir = false)
{
    if (y < 0 || y >= Chunk::HEIGHT)
        return;
    const BlockType current = context.getBlock(x, y, z);
    if ((requireAir && current == BlockType::Air) ||
        (!requireAir && airOrLeaves(current)))
        context.setBlock(x, y, z, leaves);
}

void placeVine(
    WorldGenerationContext& context,
    int x,
    int y,
    int z)
{
    if (y >= 0 && y < Chunk::HEIGHT &&
        context.getBlock(x, y, z) == BlockType::Air)
        context.setBlock(x, y, z, BlockType::Vine);
}

void addHangingVine(
    WorldGenerationContext& context,
    int x,
    int y,
    int z)
{
    placeVine(context, x, y, z);
    int remaining = 4;
    for (--y; y >= 0 && remaining > 0 &&
         context.getBlock(x, y, z) == BlockType::Air;
         --y, --remaining)
        placeVine(context, x, y, z);
}

void growLeavesLayer(
    WorldGenerationContext& context,
    int x,
    int y,
    int z,
    int width,
    BlockType leaves)
{
    const int radiusSquared = width * width;
    for (int dx = -width; dx <= width; ++dx)
        for (int dz = -width; dz <= width; ++dz)
            if (dx * dx + dz * dz <= radiusSquared)
                placeLeaf(context, x + dx, y, z + dz, leaves);
}

void growLeavesLayerStrict(
    WorldGenerationContext& context,
    int x,
    int y,
    int z,
    int width,
    BlockType leaves)
{
    const int radiusSquared = width * width;
    for (int dx = -width; dx <= width + 1; ++dx)
    {
        for (int dz = -width; dz <= width + 1; ++dz)
        {
            const int ax = dx - 1;
            const int az = dz - 1;
            if (dx * dx + dz * dz <= radiusSquared ||
                ax * ax + az * az <= radiusSquared ||
                dx * dx + az * az <= radiusSquared ||
                ax * ax + dz * dz <= radiusSquared)
                placeLeaf(context, x + dx, y, z + dz, leaves);
        }
    }
}

bool validSimpleTreeSpace(
    const WorldGenerationContext& context,
    int x,
    int y,
    int z,
    int height,
    int topRadius,
    bool allowWaterAtBase,
    bool (*canGrow)(BlockType))
{
    if (y < 1 || y + height + 1 > Chunk::HEIGHT)
        return false;

    for (int py = y; py <= y + height + 1; ++py)
    {
        int radius = 1;
        if (py == y)
            radius = 0;
        if (py >= y + height - 1)
            radius = topRadius;
        for (int px = x - radius; px <= x + radius; ++px)
        {
            for (int pz = z - radius; pz <= z + radius; ++pz)
            {
                const BlockType block = context.getBlock(px, py, pz);
                if (!canGrow(block) &&
                    !(allowWaterAtBase && py == y && block == BlockType::Water))
                    return false;
            }
        }
    }
    return true;
}

bool generateVanillaTrees(
    WorldGenerationContext& context,
    JavaRandom& random,
    int x,
    int y,
    int z,
    int minTreeHeight,
    BlockType wood,
    BlockType leaves,
    bool growVines,
    bool (*canGrow)(BlockType))
{
    const int height = random.nextInt(3) + minTreeHeight;
    if (!validSimpleTreeSpace(
            context, x, y, z, height, 2, false, canGrow))
        return false;

    const BlockType soil = context.getBlock(x, y - 1, z);
    if ((soil != BlockType::Grass && soil != BlockType::Dirt &&
         soil != BlockType::Farmland) ||
        y >= Chunk::HEIGHT - height - 1)
        return false;

    setDirtAt(context, x, y - 1, z);

    for (int py = y - 3 + height; py <= y + height; ++py)
    {
        const int layer = py - (y + height);
        const int radius = 1 - layer / 2;
        for (int px = x - radius; px <= x + radius; ++px)
        {
            const int dx = px - x;
            for (int pz = z - radius; pz <= z + radius; ++pz)
            {
                const int dz = pz - z;
                if (std::abs(dx) != radius || std::abs(dz) != radius ||
                    (random.nextInt(2) != 0 && layer != 0))
                {
                    if (airLeavesOrVine(context.getBlock(px, py, pz)))
                        context.setBlock(px, py, pz, leaves);
                }
            }
        }
    }

    for (int dy = 0; dy < height; ++dy)
    {
        const BlockType current = context.getBlock(x, y + dy, z);
        if (airLeavesOrVine(current))
        {
            context.setBlock(x, y + dy, z, wood);
            if (growVines && dy > 0)
            {
                if (random.nextInt(3) > 0 &&
                    context.getBlock(x - 1, y + dy, z) == BlockType::Air)
                    placeVine(context, x - 1, y + dy, z);
                if (random.nextInt(3) > 0 &&
                    context.getBlock(x + 1, y + dy, z) == BlockType::Air)
                    placeVine(context, x + 1, y + dy, z);
                if (random.nextInt(3) > 0 &&
                    context.getBlock(x, y + dy, z - 1) == BlockType::Air)
                    placeVine(context, x, y + dy, z - 1);
                if (random.nextInt(3) > 0 &&
                    context.getBlock(x, y + dy, z + 1) == BlockType::Air)
                    placeVine(context, x, y + dy, z + 1);
            }
        }
    }

    if (growVines)
    {
        for (int py = y - 3 + height; py <= y + height; ++py)
        {
            const int layer = py - (y + height);
            const int radius = 2 - layer / 2;
            for (int px = x - radius; px <= x + radius; ++px)
            {
                for (int pz = z - radius; pz <= z + radius; ++pz)
                {
                    if (!isLeaf(context.getBlock(px, py, pz)))
                        continue;
                    if (random.nextInt(4) == 0 &&
                        context.getBlock(px - 1, py, pz) == BlockType::Air)
                        addHangingVine(context, px - 1, py, pz);
                    if (random.nextInt(4) == 0 &&
                        context.getBlock(px + 1, py, pz) == BlockType::Air)
                        addHangingVine(context, px + 1, py, pz);
                    if (random.nextInt(4) == 0 &&
                        context.getBlock(px, py, pz - 1) == BlockType::Air)
                        addHangingVine(context, px, py, pz - 1);
                    if (random.nextInt(4) == 0 &&
                        context.getBlock(px, py, pz + 1) == BlockType::Air)
                        addHangingVine(context, px, py, pz + 1);
                }
            }
        }

        if (random.nextInt(5) == 0 && height > 5)
        {
            for (int row = 0; row < 2; ++row)
            {
                for (const Direction direction : Horizontal)
                {
                    if (random.nextInt(4 - row) == 0)
                    {
                        const int age = random.nextInt(3);
                        const int px = x - direction.x;
                        const int pz = z - direction.z;
                        if (context.getBlock(px, y + height - 5 + row, pz) == BlockType::Air)
                            context.setBlockState(
                                px, y + height - 5 + row, pz,
                                mc::content::BlockState(
                                    BlockType::Cocoa,
                                    static_cast<std::uint16_t>(age)
                                )
                            );
                    }
                }
            }
        }
    }

    return true;
}

int greatestDistance(int x, int y, int z) noexcept
{
    return std::max({std::abs(x), std::abs(y), std::abs(z)});
}

bool canGrowVanilla(BlockType block) noexcept
{
    return block == BlockType::Air || isLeaf(block) ||
           block == BlockType::Grass || block == BlockType::Dirt ||
           isLog(block) || block == BlockType::Vine;
}

bool checkHugeSpace(
    const WorldGenerationContext& context,
    int x,
    int y,
    int z,
    int height)
{
    if (y < 1 || y + height + 1 > Chunk::HEIGHT)
        return false;
    for (int dy = 0; dy <= height + 1; ++dy)
    {
        const int radius = dy == 0 ? 1 : 2;
        for (int dx = -radius; dx <= radius; ++dx)
            for (int dz = -radius; dz <= radius; ++dz)
                if (!canGrowVanilla(context.getBlock(x + dx, y + dy, z + dz)))
                    return false;
    }
    return true;
}

bool checkCanopySpace(
    const WorldGenerationContext& context,
    int x,
    int y,
    int z,
    int height)
{
    // WorldGenCanopyTree.placeTreeOfHeight differs from WorldGenHugeTrees:
    // the base checks only the trunk position, middle layers use radius 1,
    // and the top two layers use radius 2.
    if (y < 1 || y + height + 1 >= Chunk::HEIGHT)
        return false;
    for (int dy = 0; dy <= height + 1; ++dy)
    {
        int radius = 1;
        if (dy == 0)
            radius = 0;
        if (dy >= height - 1)
            radius = 2;
        for (int dx = -radius; dx <= radius; ++dx)
            for (int dz = -radius; dz <= radius; ++dz)
                if (!canGrowVanilla(context.getBlock(x + dx, y + dy, z + dz)))
                    return false;
    }
    return true;
}

bool prepareHugeSoil(
    WorldGenerationContext& context,
    int x,
    int y,
    int z)
{
    if (y < 2)
        return false;
    const BlockType soil = context.getBlock(x, y - 1, z);
    if (soil != BlockType::Grass && soil != BlockType::Dirt)
        return false;
    setDirtAt(context, x, y - 1, z);
    setDirtAt(context, x + 1, y - 1, z);
    setDirtAt(context, x, y - 1, z + 1);
    setDirtAt(context, x + 1, y - 1, z + 1);
    return true;
}

void placePodzolAt(
    WorldGenerationContext& context,
    int x,
    int y,
    int z)
{
    for (int offset = 2; offset >= -3; --offset)
    {
        const int py = y + offset;
        const BlockType block = context.getBlock(x, py, z);
        if (block == BlockType::Grass || block == BlockType::Dirt)
        {
            context.setBlock(x, py, z, BlockType::Podzol);
            break;
        }
        if (block != BlockType::Air && offset < 0)
            break;
    }
}

void placePodzolCircle(
    WorldGenerationContext& context,
    int x,
    int y,
    int z)
{
    for (int dx = -2; dx <= 2; ++dx)
        for (int dz = -2; dz <= 2; ++dz)
            if (std::abs(dx) != 2 || std::abs(dz) != 2)
                placePodzolAt(context, x + dx, y, z + dz);
}
}

bool TreeGenerator::canReplace(BlockType block) noexcept
{
    return canGrowVanilla(block);
}

bool TreeGenerator::canOccupy(
    const WorldGenerationContext& context,
    int x,
    int y,
    int z) noexcept
{
    return y >= 0 && y < Chunk::HEIGHT &&
           canReplace(context.getBlock(x, y, z));
}

bool TreeGenerator::hasValidSoil(
    const WorldGenerationContext& context,
    int x,
    int y,
    int z) noexcept
{
    if (y <= 0)
        return false;
    const BlockType soil = context.getBlock(x, y - 1, z);
    return soil == BlockType::Grass || soil == BlockType::Dirt ||
           soil == BlockType::Farmland;
}

bool TreeGenerator::generateForBiome(
    WorldGenerationContext& context,
    JavaRandom& random,
    BiomeId biome,
    int x,
    int y,
    int z) const
{
    const BiomeDefinition* definition = BiomeRegistry::active().find(biome);
    const TreeFeature feature = definition == nullptr
        ? TreeFeature::Oak
        : definition->treeFeature;

    switch (feature)
    {
        case TreeFeature::None:
            return false;
        case TreeFeature::OakOnly:
            return generateOak(context, random, x, y, z);
        case TreeFeature::Plains:
            return random.nextInt(3) == 0
                ? generateBigOak(context, random, x, y, z)
                : generateOak(context, random, x, y, z);
        case TreeFeature::Forest:
            if (random.nextInt(5) == 0)
                return generateBirch(context, random, x, y, z);
            return random.nextInt(10) == 0
                ? generateBigOak(context, random, x, y, z)
                : generateOak(context, random, x, y, z);
        case TreeFeature::Birch:
            return generateBirch(context, random, x, y, z);
        case TreeFeature::TallBirch:
            return generateBirch(
                context, random, x, y, z, random.nextBoolean()
            );
        case TreeFeature::Spruce:
            // BiomeSnow uses WorldGenTaiga2.
            return generateSpruce(context, random, x, y, z);
        case TreeFeature::Taiga:
            return random.nextInt(3) == 0
                ? generatePine(context, random, x, y, z)
                : generateSpruce(context, random, x, y, z);
        case TreeFeature::MegaTaiga:
        case TreeFeature::MegaSpruceTaiga:
            if (random.nextInt(3) == 0)
            {
                const bool useBaseHeight =
                    feature == TreeFeature::MegaSpruceTaiga ||
                    random.nextInt(13) == 0;
                return generateMegaPine(
                    context, random, x, y, z, useBaseHeight
                );
            }
            return random.nextInt(3) == 0
                ? generatePine(context, random, x, y, z)
                : generateSpruce(context, random, x, y, z);
        case TreeFeature::Jungle:
        case TreeFeature::JungleEdge:
            if (random.nextInt(10) == 0)
                return generateBigOak(context, random, x, y, z);
            if (random.nextInt(2) == 0)
                return generateJungleShrub(context, random, x, y, z);
            if (feature == TreeFeature::Jungle && random.nextInt(3) == 0)
                return generateMegaJungle(context, random, x, y, z);
            return generateJungle(context, random, x, y, z);
        case TreeFeature::Savanna:
            return random.nextInt(5) > 0
                ? generateAcacia(context, random, x, y, z)
                : generateOak(context, random, x, y, z);
        case TreeFeature::RoofedForest:
            if (random.nextInt(3) > 0)
                return generateDarkOak(context, random, x, y, z);
            if (random.nextInt(5) == 0)
                return generateBirch(context, random, x, y, z);
            return random.nextInt(10) == 0
                ? generateBigOak(context, random, x, y, z)
                : generateOak(context, random, x, y, z);
        case TreeFeature::Hills:
            // BiomeHills returns spruce two times out of three, otherwise the
            // base biome's 1-in-10 big oak / normal oak choice.
            if (random.nextInt(3) > 0)
                return generateSpruce(context, random, x, y, z);
            return random.nextInt(10) == 0
                ? generateBigOak(context, random, x, y, z)
                : generateOak(context, random, x, y, z);
        case TreeFeature::Swamp:
            return generateSwamp(context, random, x, y, z);
        case TreeFeature::Oak:
            return random.nextInt(10) == 0
                ? generateBigOak(context, random, x, y, z)
                : generateOak(context, random, x, y, z);
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
    return generateVanillaTrees(
        context, random, x, y, z,
        4, BlockType::OakLog, BlockType::OakLeaves,
        false, &canGrowVanilla
    );
}

bool TreeGenerator::generateBirch(
    WorldGenerationContext& context,
    JavaRandom& random,
    int x,
    int y,
    int z,
    bool extraRandomHeight) const
{
    int height = random.nextInt(3) + 5;
    if (extraRandomHeight)
        height += random.nextInt(7);

    if (!validSimpleTreeSpace(
            context, x, y, z, height, 2, false, &canGrowVanilla))
        return false;

    const BlockType soil = context.getBlock(x, y - 1, z);
    if ((soil != BlockType::Grass && soil != BlockType::Dirt &&
         soil != BlockType::Farmland) ||
        y >= Chunk::HEIGHT - height - 1)
        return false;

    setDirtAt(context, x, y - 1, z);
    for (int py = y - 3 + height; py <= y + height; ++py)
    {
        const int layer = py - (y + height);
        const int radius = 1 - layer / 2;
        for (int px = x - radius; px <= x + radius; ++px)
        {
            const int dx = px - x;
            for (int pz = z - radius; pz <= z + radius; ++pz)
            {
                const int dz = pz - z;
                if (std::abs(dx) != radius || std::abs(dz) != radius ||
                    (random.nextInt(2) != 0 && layer != 0))
                    placeLeaf(context, px, py, pz, BlockType::BirchLeaves);
            }
        }
    }
    for (int dy = 0; dy < height; ++dy)
        if (airOrLeaves(context.getBlock(x, y + dy, z)))
            context.setBlock(x, y + dy, z, BlockType::BirchLog);
    return true;
}

bool TreeGenerator::generateSpruce(
    WorldGenerationContext& context,
    JavaRandom& random,
    int x,
    int y,
    int z) const
{
    // WorldGenTaiga2.
    const int height = random.nextInt(4) + 6;
    const int bare = 1 + random.nextInt(2);
    const int crownHeight = height - bare;
    const int maxRadius = 2 + random.nextInt(2);
    if (y < 1 || y + height + 1 > Chunk::HEIGHT)
        return false;

    for (int py = y; py <= y + height + 1; ++py)
    {
        const int radius = py - y < bare ? 0 : maxRadius;
        for (int px = x - radius; px <= x + radius; ++px)
            for (int pz = z - radius; pz <= z + radius; ++pz)
                if (!airOrLeaves(context.getBlock(px, py, pz)))
                    return false;
    }

    const BlockType soil = context.getBlock(x, y - 1, z);
    if ((soil != BlockType::Grass && soil != BlockType::Dirt &&
         soil != BlockType::Farmland) ||
        y >= Chunk::HEIGHT - height - 1)
        return false;
    setDirtAt(context, x, y - 1, z);

    int radius = random.nextInt(2);
    int threshold = 1;
    int previous = 0;
    for (int layer = 0; layer <= crownHeight; ++layer)
    {
        const int py = y + height - layer;
        for (int px = x - radius; px <= x + radius; ++px)
        {
            const int dx = px - x;
            for (int pz = z - radius; pz <= z + radius; ++pz)
            {
                const int dz = pz - z;
                if (std::abs(dx) != radius || std::abs(dz) != radius || radius <= 0)
                {
                    if (!isSolid(context.getBlock(px, py, pz)))
                        context.setBlock(px, py, pz, BlockType::SpruceLeaves);
                }
            }
        }
        if (radius >= threshold)
        {
            radius = previous;
            previous = 1;
            ++threshold;
            if (threshold > maxRadius)
                threshold = maxRadius;
        }
        else
            ++radius;
    }

    const int trim = random.nextInt(3);
    for (int dy = 0; dy < height - trim; ++dy)
        if (airOrLeaves(context.getBlock(x, y + dy, z)))
            context.setBlock(x, y + dy, z, BlockType::SpruceLog);
    return true;
}

bool TreeGenerator::generatePine(
    WorldGenerationContext& context,
    JavaRandom& random,
    int x,
    int y,
    int z) const
{
    // WorldGenTaiga1.
    const int height = random.nextInt(5) + 7;
    const int bare = height - random.nextInt(2) - 3;
    const int crownHeight = height - bare;
    const int maxRadius = 1 + random.nextInt(crownHeight + 1);
    if (y < 1 || y + height + 1 > Chunk::HEIGHT)
        return false;

    for (int py = y; py <= y + height + 1; ++py)
    {
        const int radius = py - y < bare ? 0 : maxRadius;
        for (int px = x - radius; px <= x + radius; ++px)
            for (int pz = z - radius; pz <= z + radius; ++pz)
                if (!canGrowVanilla(context.getBlock(px, py, pz)))
                    return false;
    }

    const BlockType soil = context.getBlock(x, y - 1, z);
    if ((soil != BlockType::Grass && soil != BlockType::Dirt) ||
        y >= Chunk::HEIGHT - height - 1)
        return false;
    setDirtAt(context, x, y - 1, z);

    int radius = 0;
    for (int py = y + height; py >= y + bare; --py)
    {
        for (int px = x - radius; px <= x + radius; ++px)
        {
            const int dx = px - x;
            for (int pz = z - radius; pz <= z + radius; ++pz)
            {
                const int dz = pz - z;
                if (std::abs(dx) != radius || std::abs(dz) != radius || radius <= 0)
                {
                    if (!isSolid(context.getBlock(px, py, pz)))
                        context.setBlock(px, py, pz, BlockType::SpruceLeaves);
                }
            }
        }
        if (radius >= 1 && py == y + bare + 1)
            --radius;
        else if (radius < maxRadius)
            ++radius;
    }

    for (int dy = 0; dy < height - 1; ++dy)
        if (airOrLeaves(context.getBlock(x, y + dy, z)))
            context.setBlock(x, y + dy, z, BlockType::SpruceLog);
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
    if (y < 1 || y + height + 1 > Chunk::HEIGHT)
        return false;

    for (int py = y; py <= y + height + 1; ++py)
    {
        int radius = 1;
        if (py == y)
            radius = 0;
        if (py >= y + height - 1)
            radius = 3;
        for (int px = x - radius; px <= x + radius; ++px)
        {
            for (int pz = z - radius; pz <= z + radius; ++pz)
            {
                const BlockType block = context.getBlock(px, py, pz);
                if (!airOrLeaves(block) && block != BlockType::Water)
                    return false;
                if (block == BlockType::Water && py > y)
                    return false;
            }
        }
    }

    const BlockType soil = context.getBlock(x, y - 1, z);
    if ((soil != BlockType::Grass && soil != BlockType::Dirt) ||
        y >= Chunk::HEIGHT - height - 1)
        return false;
    setDirtAt(context, x, y - 1, z);

    for (int py = y - 3 + height; py <= y + height; ++py)
    {
        const int layer = py - (y + height);
        const int radius = 2 - layer / 2;
        for (int px = x - radius; px <= x + radius; ++px)
        {
            const int dx = px - x;
            for (int pz = z - radius; pz <= z + radius; ++pz)
            {
                const int dz = pz - z;
                if (std::abs(dx) != radius || std::abs(dz) != radius ||
                    (random.nextInt(2) != 0 && layer != 0))
                {
                    if (!isSolid(context.getBlock(px, py, pz)))
                        context.setBlock(px, py, pz, BlockType::OakLeaves);
                }
            }
        }
    }

    for (int dy = 0; dy < height; ++dy)
    {
        const BlockType block = context.getBlock(x, y + dy, z);
        if (airOrLeaves(block) || block == BlockType::Water)
            context.setBlock(x, y + dy, z, BlockType::OakLog);
    }

    for (int py = y - 3 + height; py <= y + height; ++py)
    {
        const int layer = py - (y + height);
        const int radius = 2 - layer / 2;
        for (int px = x - radius; px <= x + radius; ++px)
        {
            for (int pz = z - radius; pz <= z + radius; ++pz)
            {
                if (!isLeaf(context.getBlock(px, py, pz)))
                    continue;
                if (random.nextInt(4) == 0 && context.getBlock(px - 1, py, pz) == BlockType::Air)
                    addHangingVine(context, px - 1, py, pz);
                if (random.nextInt(4) == 0 && context.getBlock(px + 1, py, pz) == BlockType::Air)
                    addHangingVine(context, px + 1, py, pz);
                if (random.nextInt(4) == 0 && context.getBlock(px, py, pz - 1) == BlockType::Air)
                    addHangingVine(context, px, py, pz - 1);
                if (random.nextInt(4) == 0 && context.getBlock(px, py, pz + 1) == BlockType::Air)
                    addHangingVine(context, px, py, pz + 1);
            }
        }
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
    const int minHeight = 4 + random.nextInt(7);
    return generateVanillaTrees(
        context, random, x, y, z,
        minHeight, BlockType::JungleLog, BlockType::JungleLeaves,
        true, &canGrowVanilla
    );
}

bool TreeGenerator::generateJungleShrub(
    WorldGenerationContext& context,
    JavaRandom& random,
    int x,
    int y,
    int z) const
{
    while (y > 0)
    {
        const BlockType block = context.getBlock(x, y, z);
        if (block != BlockType::Air && !isLeaf(block))
            break;
        --y;
    }
    const BlockType soil = context.getBlock(x, y, z);
    if (soil != BlockType::Dirt && soil != BlockType::Grass)
        return false;
    ++y;
    context.setBlock(x, y, z, BlockType::JungleLog);
    for (int py = y; py <= y + 2; ++py)
    {
        const int layer = py - y;
        const int radius = 2 - layer;
        for (int px = x - radius; px <= x + radius; ++px)
            for (int pz = z - radius; pz <= z + radius; ++pz)
                if (random.nextInt(2) != 0 ||
                    (std::abs(px - x) != radius || std::abs(pz - z) != radius))
                    placeLeaf(context, px, py, pz, BlockType::OakLeaves);
    }
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
    if (!validSimpleTreeSpace(
            context, x, y, z, height, 2, false, &canGrowVanilla))
        return false;
    const BlockType soil = context.getBlock(x, y - 1, z);
    if ((soil != BlockType::Grass && soil != BlockType::Dirt) ||
        y >= Chunk::HEIGHT - height - 1)
        return false;
    setDirtAt(context, x, y - 1, z);

    const Direction mainDirection = Horizontal[static_cast<std::size_t>(random.nextInt(4))];
    const int bendStart = height - random.nextInt(4) - 1;
    int bendRemaining = 3 - random.nextInt(3);
    int trunkX = x;
    int trunkZ = z;
    int topY = 0;
    for (int dy = 0; dy < height; ++dy)
    {
        const int py = y + dy;
        if (dy >= bendStart && bendRemaining > 0)
        {
            trunkX += mainDirection.x;
            trunkZ += mainDirection.z;
            --bendRemaining;
        }
        if (airOrLeaves(context.getBlock(trunkX, py, trunkZ)))
        {
            context.setBlock(trunkX, py, trunkZ, BlockType::AcaciaLog);
            topY = py;
        }
    }

    for (int dx = -3; dx <= 3; ++dx)
        for (int dz = -3; dz <= 3; ++dz)
            if (std::abs(dx) != 3 || std::abs(dz) != 3)
                placeLeaf(context, trunkX + dx, topY, trunkZ + dz, BlockType::AcaciaLeaves);
    for (int dx = -1; dx <= 1; ++dx)
        for (int dz = -1; dz <= 1; ++dz)
            placeLeaf(context, trunkX + dx, topY + 1, trunkZ + dz, BlockType::AcaciaLeaves);
    placeLeaf(context, trunkX + 2, topY + 1, trunkZ, BlockType::AcaciaLeaves);
    placeLeaf(context, trunkX - 2, topY + 1, trunkZ, BlockType::AcaciaLeaves);
    placeLeaf(context, trunkX, topY + 1, trunkZ + 2, BlockType::AcaciaLeaves);
    placeLeaf(context, trunkX, topY + 1, trunkZ - 2, BlockType::AcaciaLeaves);

    int branchX = x;
    int branchZ = z;
    const Direction secondDirection = Horizontal[static_cast<std::size_t>(random.nextInt(4))];
    if (secondDirection.x != mainDirection.x || secondDirection.z != mainDirection.z)
    {
        const int branchStart = bendStart - random.nextInt(2) - 1;
        int remaining = 1 + random.nextInt(3);
        int branchTop = 0;
        for (int dy = branchStart; dy < height && remaining > 0; ++dy, --remaining)
        {
            if (dy < 1)
                continue;
            const int py = y + dy;
            branchX += secondDirection.x;
            branchZ += secondDirection.z;
            if (airOrLeaves(context.getBlock(branchX, py, branchZ)))
            {
                context.setBlock(branchX, py, branchZ, BlockType::AcaciaLog);
                branchTop = py;
            }
        }
        if (branchTop > 0)
        {
            for (int dx = -2; dx <= 2; ++dx)
                for (int dz = -2; dz <= 2; ++dz)
                    if (std::abs(dx) != 2 || std::abs(dz) != 2)
                        placeLeaf(context, branchX + dx, branchTop, branchZ + dz, BlockType::AcaciaLeaves);
            for (int dx = -1; dx <= 1; ++dx)
                for (int dz = -1; dz <= 1; ++dz)
                    placeLeaf(context, branchX + dx, branchTop + 1, branchZ + dz, BlockType::AcaciaLeaves);
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
    const int height = random.nextInt(3) + random.nextInt(2) + 6;
    if (!checkCanopySpace(context, x, y, z, height) ||
        !prepareHugeSoil(context, x, y, z))
        return false;

    const Direction direction = Horizontal[static_cast<std::size_t>(random.nextInt(4))];
    const int bendStart = height - random.nextInt(4);
    int bend = 2 - random.nextInt(3);
    int trunkX = x;
    int trunkZ = z;
    const int leafY = y + height - 1;

    for (int dy = 0; dy < height; ++dy)
    {
        if (dy >= bendStart && bend > 0)
        {
            trunkX += direction.x;
            trunkZ += direction.z;
            --bend;
        }
        const int py = y + dy;
        if (airOrLeaves(context.getBlock(trunkX, py, trunkZ)))
        {
            context.setBlock(trunkX, py, trunkZ, BlockType::DarkOakLog);
            context.setBlock(trunkX + 1, py, trunkZ, BlockType::DarkOakLog);
            context.setBlock(trunkX, py, trunkZ + 1, BlockType::DarkOakLog);
            context.setBlock(trunkX + 1, py, trunkZ + 1, BlockType::DarkOakLog);
        }
    }

    for (int dx = -2; dx <= 0; ++dx)
    {
        for (int dz = -2; dz <= 0; ++dz)
        {
            placeLeaf(context, trunkX + dx, leafY - 1, trunkZ + dz, BlockType::DarkOakLeaves, true);
            placeLeaf(context, trunkX + 1 - dx, leafY - 1, trunkZ + dz, BlockType::DarkOakLeaves, true);
            placeLeaf(context, trunkX + dx, leafY - 1, trunkZ + 1 - dz, BlockType::DarkOakLeaves, true);
            placeLeaf(context, trunkX + 1 - dx, leafY - 1, trunkZ + 1 - dz, BlockType::DarkOakLeaves, true);
            if ((dx > -2 || dz > -1) && (dx != -1 || dz != -2))
            {
                placeLeaf(context, trunkX + dx, leafY + 1, trunkZ + dz, BlockType::DarkOakLeaves, true);
                placeLeaf(context, trunkX + 1 - dx, leafY + 1, trunkZ + dz, BlockType::DarkOakLeaves, true);
                placeLeaf(context, trunkX + dx, leafY + 1, trunkZ + 1 - dz, BlockType::DarkOakLeaves, true);
                placeLeaf(context, trunkX + 1 - dx, leafY + 1, trunkZ + 1 - dz, BlockType::DarkOakLeaves, true);
            }
        }
    }
    if (random.nextBoolean())
    {
        placeLeaf(context, trunkX, leafY + 2, trunkZ, BlockType::DarkOakLeaves, true);
        placeLeaf(context, trunkX + 1, leafY + 2, trunkZ, BlockType::DarkOakLeaves, true);
        placeLeaf(context, trunkX, leafY + 2, trunkZ + 1, BlockType::DarkOakLeaves, true);
        placeLeaf(context, trunkX + 1, leafY + 2, trunkZ + 1, BlockType::DarkOakLeaves, true);
    }
    for (int dx = -3; dx <= 4; ++dx)
        for (int dz = -3; dz <= 4; ++dz)
            if ((dx != -3 || dz != -3) && (dx != -3 || dz != 4) &&
                (dx != 4 || dz != -3) && (dx != 4 || dz != 4) &&
                (std::abs(dx) < 3 || std::abs(dz) < 3))
                placeLeaf(context, trunkX + dx, leafY, trunkZ + dz, BlockType::DarkOakLeaves, true);

    for (int dx = -1; dx <= 2; ++dx)
    {
        for (int dz = -1; dz <= 2; ++dz)
        {
            if ((dx < 0 || dx > 1 || dz < 0 || dz > 1) && random.nextInt(3) == 0)
            {
                const int branchHeight = random.nextInt(3) + 2;
                for (int dy = 0; dy < branchHeight; ++dy)
                    context.setBlock(x + dx, leafY - dy - 1, z + dz, BlockType::DarkOakLog);
                for (int lx = -1; lx <= 1; ++lx)
                    for (int lz = -1; lz <= 1; ++lz)
                        placeLeaf(context, trunkX + dx + lx, leafY, trunkZ + dz + lz, BlockType::DarkOakLeaves, true);
                for (int lx = -2; lx <= 2; ++lx)
                    for (int lz = -2; lz <= 2; ++lz)
                        if (std::abs(lx) != 2 || std::abs(lz) != 2)
                            placeLeaf(context, trunkX + dx + lx, leafY - 1, trunkZ + dz + lz, BlockType::DarkOakLeaves, true);
            }
        }
    }
    return true;
}

bool TreeGenerator::generateMegaPine(
    WorldGenerationContext& context,
    JavaRandom& random,
    int x,
    int y,
    int z,
    bool useBaseHeight) const
{
    int height = random.nextInt(3) + 13;
    height += random.nextInt(15);
    if (!checkHugeSpace(context, x, y, z, height) ||
        !prepareHugeSoil(context, x, y, z))
        return false;

    const int crownHeight = random.nextInt(5) + (useBaseHeight ? 13 : 3);
    int previousRadius = 0;
    for (int py = y + height - crownHeight; py <= y + height; ++py)
    {
        const int down = y + height - py;
        const int radius = static_cast<int>(std::floor(
            static_cast<float>(down) / crownHeight * 3.5F
        ));
        const int actualRadius = radius +
            (down > 0 && radius == previousRadius && (py & 1) == 0 ? 1 : 0);
        growLeavesLayerStrict(
            context, x, py, z, actualRadius, BlockType::SpruceLeaves
        );
        previousRadius = radius;
    }

    for (int dy = 0; dy < height; ++dy)
    {
        if (airOrLeaves(context.getBlock(x, y + dy, z)))
            context.setBlock(x, y + dy, z, BlockType::SpruceLog);
        if (dy < height - 1)
        {
            if (airOrLeaves(context.getBlock(x + 1, y + dy, z)))
                context.setBlock(x + 1, y + dy, z, BlockType::SpruceLog);
            if (airOrLeaves(context.getBlock(x + 1, y + dy, z + 1)))
                context.setBlock(x + 1, y + dy, z + 1, BlockType::SpruceLog);
            if (airOrLeaves(context.getBlock(x, y + dy, z + 1)))
                context.setBlock(x, y + dy, z + 1, BlockType::SpruceLog);
        }
    }

    placePodzolCircle(context, x - 1, y, z - 1);
    placePodzolCircle(context, x + 2, y, z - 1);
    placePodzolCircle(context, x - 1, y, z + 2);
    placePodzolCircle(context, x + 2, y, z + 2);
    for (int i = 0; i < 5; ++i)
    {
        const int value = random.nextInt(64);
        const int px = value % 8;
        const int pz = value / 8;
        if (px == 0 || px == 7 || pz == 0 || pz == 7)
            placePodzolCircle(context, x - 3 + px, y, z - 3 + pz);
    }
    return true;
}

bool TreeGenerator::generateMegaJungle(
    WorldGenerationContext& context,
    JavaRandom& random,
    int x,
    int y,
    int z) const
{
    int height = random.nextInt(3) + 10;
    height += random.nextInt(20);
    if (!checkHugeSpace(context, x, y, z, height) ||
        !prepareHugeSoil(context, x, y, z))
        return false;

    for (int offset = -2; offset <= 0; ++offset)
        growLeavesLayerStrict(
            context, x, y + height + offset, z,
            3 - offset, BlockType::JungleLeaves
        );

    for (int branchY = y + height - 2 - random.nextInt(4);
         branchY > y + height / 2;
         branchY -= 2 + random.nextInt(4))
    {
        const float angle = random.nextFloat() *
            static_cast<float>(std::numbers::pi * 2.0);
        int endX = x + static_cast<int>(0.5F + std::cos(angle) * 4.0F);
        int endZ = z + static_cast<int>(0.5F + std::sin(angle) * 4.0F);
        for (int step = 0; step < 5; ++step)
        {
            endX = x + static_cast<int>(1.5F + std::cos(angle) * step);
            endZ = z + static_cast<int>(1.5F + std::sin(angle) * step);
            context.setBlock(
                endX, branchY - 3 + step / 2, endZ,
                BlockType::JungleLog
            );
        }
        const int leafDepth = 1 + random.nextInt(2);
        for (int py = branchY - leafDepth; py <= branchY; ++py)
            growLeavesLayer(
                context, endX, py, endZ,
                1 - (py - branchY), BlockType::JungleLeaves
            );
    }

    for (int dy = 0; dy < height; ++dy)
    {
        const int py = y + dy;
        if (canGrowVanilla(context.getBlock(x, py, z)))
        {
            context.setBlock(x, py, z, BlockType::JungleLog);
            if (dy > 0)
            {
                if (random.nextInt(3) > 0 && context.getBlock(x - 1, py, z) == BlockType::Air)
                    placeVine(context, x - 1, py, z);
                if (random.nextInt(3) > 0 && context.getBlock(x, py, z - 1) == BlockType::Air)
                    placeVine(context, x, py, z - 1);
            }
        }
        if (dy < height - 1)
        {
            for (const auto [dx, dz] : {
                     std::pair{1, 0}, std::pair{1, 1}, std::pair{0, 1}})
            {
                if (canGrowVanilla(context.getBlock(x + dx, py, z + dz)))
                {
                    context.setBlock(x + dx, py, z + dz, BlockType::JungleLog);
                    if (dy > 0)
                    {
                        if (dx == 1 && random.nextInt(3) > 0 &&
                            context.getBlock(x + dx + 1, py, z + dz) == BlockType::Air)
                            placeVine(context, x + dx + 1, py, z + dz);
                        if (dz == 1 && random.nextInt(3) > 0 &&
                            context.getBlock(x + dx, py, z + dz + 1) == BlockType::Air)
                            placeVine(context, x + dx, py, z + dz + 1);
                    }
                }
            }
        }
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
    if (!hasValidSoil(context, x, y, z))
        return false;

    // Direct C++ translation of WorldGenBigTree's decoration-mode settings.
    JavaRandom treeRandom(random.nextLong());
    int heightLimit = 5 + treeRandom.nextInt(12);
    constexpr double heightAttenuation = 0.618;
    constexpr double branchSlope = 0.381;
    constexpr double scaleWidth = 1.0;
    constexpr double leafDensity = 1.0;
    constexpr int leafDistanceLimit = 5;

    const auto lineBlocked = [&context](
        int ax, int ay, int az,
        int bx, int by, int bz)
    {
        const int dx = bx - ax;
        const int dy = by - ay;
        const int dz = bz - az;
        const int distance = greatestDistance(dx, dy, dz);
        if (distance == 0)
            return -1;
        const float sx = static_cast<float>(dx) / distance;
        const float sy = static_cast<float>(dy) / distance;
        const float sz = static_cast<float>(dz) / distance;
        for (int step = 0; step <= distance; ++step)
        {
            const int px = ax + static_cast<int>(std::floor(0.5F + step * sx));
            const int py = ay + static_cast<int>(std::floor(0.5F + step * sy));
            const int pz = az + static_cast<int>(std::floor(0.5F + step * sz));
            if (!canGrowVanilla(context.getBlock(px, py, pz)))
                return step;
        }
        return -1;
    };

    int obstruction = lineBlocked(x, y, z, x, y + heightLimit - 1, z);
    if (obstruction != -1)
    {
        if (obstruction < 6)
            return false;
        heightLimit = obstruction;
    }
    setDirtAt(context, x, y - 1, z);

    int trunkHeight = static_cast<int>(heightLimit * heightAttenuation);
    if (trunkHeight >= heightLimit)
        trunkHeight = heightLimit - 1;
    int nodesPerLayer = static_cast<int>(
        1.382 + std::pow(leafDensity * heightLimit / 13.0, 2.0)
    );
    nodesPerLayer = std::max(1, nodesPerLayer);
    const int branchCeiling = y + trunkHeight;

    struct Node { int x, y, z, branchBase; };
    std::vector<Node> nodes;
    int layer = heightLimit - leafDistanceLimit;
    nodes.push_back({x, y + layer, z, branchCeiling});

    const auto layerSize = [heightLimit](int py)
    {
        if (static_cast<float>(py) < heightLimit * 0.3F)
            return -1.0F;
        const float half = heightLimit / 2.0F;
        const float offset = half - py;
        if (std::abs(offset) >= half)
            return 0.0F;
        float value = std::sqrt(half * half - offset * offset);
        if (offset == 0.0F)
            value = half;
        return value * 0.5F;
    };

    for (; layer >= 0; --layer)
    {
        const float size = layerSize(layer);
        if (size < 0.0F)
            continue;
        for (int attempt = 0; attempt < nodesPerLayer; ++attempt)
        {
            const double distance = scaleWidth * size *
                (static_cast<double>(treeRandom.nextFloat()) + 0.328);
            const double angle = static_cast<double>(treeRandom.nextFloat()) *
                std::numbers::pi * 2.0;
            const int nx = x + static_cast<int>(std::floor(distance * std::sin(angle) + 0.5));
            const int nz = z + static_cast<int>(std::floor(distance * std::cos(angle) + 0.5));
            const int ny = y + layer - 1;
            if (lineBlocked(nx, ny, nz, nx, ny + leafDistanceLimit, nz) != -1)
                continue;
            const int dx = x - nx;
            const int dz = z - nz;
            const double branchY = ny - std::sqrt(static_cast<double>(dx * dx + dz * dz)) * branchSlope;
            const int baseY = branchY > branchCeiling
                ? branchCeiling : static_cast<int>(branchY);
            if (lineBlocked(x, baseY, z, nx, ny, nz) == -1)
                nodes.push_back({nx, ny, nz, baseY});
        }
    }

    const auto crossSection = [&context](int cx, int cy, int cz, float radius)
    {
        const int r = static_cast<int>(radius + 0.618);
        for (int dx = -r; dx <= r; ++dx)
            for (int dz = -r; dz <= r; ++dz)
                if (std::pow(std::abs(dx) + 0.5, 2.0) +
                    std::pow(std::abs(dz) + 0.5, 2.0) <= radius * radius)
                    placeLeaf(context, cx + dx, cy, cz + dz, BlockType::OakLeaves);
    };

    for (const Node& node : nodes)
    {
        for (int dy = 0; dy < leafDistanceLimit; ++dy)
        {
            const float radius = (dy == 0 || dy == leafDistanceLimit - 1)
                ? 2.0F : 3.0F;
            crossSection(node.x, node.y + dy, node.z, radius);
        }
    }

    const auto limb = [&context](
        int ax, int ay, int az,
        int bx, int by, int bz)
    {
        const int dx = bx - ax;
        const int dy = by - ay;
        const int dz = bz - az;
        const int distance = greatestDistance(dx, dy, dz);
        if (distance <= 0)
            return;
        const float sx = static_cast<float>(dx) / distance;
        const float sy = static_cast<float>(dy) / distance;
        const float sz = static_cast<float>(dz) / distance;
        for (int step = 0; step <= distance; ++step)
        {
            const int px = ax + static_cast<int>(std::floor(0.5F + step * sx));
            const int py = ay + static_cast<int>(std::floor(0.5F + step * sy));
            const int pz = az + static_cast<int>(std::floor(0.5F + step * sz));
            context.setBlock(px, py, pz, BlockType::OakLog);
        }
    };

    limb(x, y, z, x, y + trunkHeight, z);
    for (const Node& node : nodes)
    {
        if (node.branchBase == node.y)
            continue;
        if (static_cast<double>(node.branchBase - y) >= heightLimit * 0.2)
            limb(x, node.branchBase, z, node.x, node.y, node.z);
    }
    return true;
}
