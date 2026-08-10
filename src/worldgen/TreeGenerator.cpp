#include "worldgen/TreeGenerator.h"

#include "Chunk.h"
#include "worldgen/JavaRandom.h"
#include "worldgen/WorldGenerationContext.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <vector>

namespace
{
void placeLeaf(
    WorldGenerationContext& context,
    int x,
    int y,
    int z,
    BlockType leaves)
{
    const BlockType current = context.getBlock(x, y, z);
    if (current == BlockType::Air || isLeaf(current))
        context.setBlock(x, y, z, leaves);
}

void growLeafCircle(
    WorldGenerationContext& context,
    int centerX,
    int y,
    int centerZ,
    int radius,
    BlockType leaves)
{
    const int radiusSquared = radius * radius;
    for (int dx = -radius; dx <= radius; ++dx)
    {
        for (int dz = -radius; dz <= radius; ++dz)
        {
            if (dx * dx + dz * dz <= radiusSquared)
                placeLeaf(context, centerX + dx, y, centerZ + dz, leaves);
        }
    }
}

void growStrictLeafLayer(
    WorldGenerationContext& context,
    int centerX,
    int y,
    int centerZ,
    int width,
    BlockType leaves)
{
    const int widthSquared = width * width;
    for (int dx = -width; dx <= width + 1; ++dx)
    {
        for (int dz = -width; dz <= width + 1; ++dz)
        {
            const int nearX = dx - 1;
            const int nearZ = dz - 1;
            if (dx * dx + dz * dz <= widthSquared ||
                nearX * nearX + nearZ * nearZ <= widthSquared ||
                dx * dx + nearZ * nearZ <= widthSquared ||
                nearX * nearX + dz * dz <= widthSquared)
            {
                placeLeaf(
                    context, centerX + dx, y, centerZ + dz, leaves
                );
            }
        }
    }
}
}

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
        case TreeFeature::OakOnly:
            return generateOak(context, random, worldX, worldY, worldZ);
        case TreeFeature::Plains:
            return random.nextInt(3) == 0
                ? generateBigOak(context, random, worldX, worldY, worldZ)
                : generateOak(context, random, worldX, worldY, worldZ);
        case TreeFeature::Forest:
            if (random.nextInt(5) == 0)
                return generateBirch(
                    context, random, worldX, worldY, worldZ
                );
            return random.nextInt(10) == 0
                ? generateBigOak(context, random, worldX, worldY, worldZ)
                : generateOak(context, random, worldX, worldY, worldZ);
        case TreeFeature::Birch:
            return generateBirch(
                context, random, worldX, worldY, worldZ
            );
        case TreeFeature::TallBirch:
            return generateBirch(
                context, random, worldX, worldY, worldZ,
                random.nextBoolean()
            );
        case TreeFeature::Spruce:
            return generateSpruce(
                context, random, worldX, worldY, worldZ
            );
        case TreeFeature::Taiga:
            return random.nextInt(3) == 0
                ? generatePine(context, random, worldX, worldY, worldZ)
                : generateSpruce(context, random, worldX, worldY, worldZ);
        case TreeFeature::MegaTaiga:
        case TreeFeature::MegaSpruceTaiga:
            if (random.nextInt(3) == 0)
            {
                const bool megaSpruce =
                    feature == TreeFeature::MegaSpruceTaiga ||
                    random.nextInt(13) == 0;
                return generateMegaPine(
                    context, random, worldX, worldY, worldZ, megaSpruce
                );
            }
            return random.nextInt(3) == 0
                ? generatePine(context, random, worldX, worldY, worldZ)
                : generateSpruce(context, random, worldX, worldY, worldZ);
        case TreeFeature::Jungle:
        case TreeFeature::JungleEdge:
            if (random.nextInt(10) == 0)
                return generateBigOak(
                    context, random, worldX, worldY, worldZ
                );
            if (random.nextInt(2) == 0)
                return generateJungleShrub(
                    context, random, worldX, worldY, worldZ
                );
            if (feature == TreeFeature::Jungle && random.nextInt(3) == 0)
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

                if ((!corner ||
                     (random.nextInt(2) != 0 && layer != 0)) &&
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
    if (y < 1 || !hasValidSoil(context, x, y, z))
        return false;

    JavaRandom treeRandom(random.nextLong());
    int heightLimit = 5 + treeRandom.nextInt(12);

    const auto greatestDistance = [](int dx, int dy, int dz)
    {
        return std::max({std::abs(dx), std::abs(dy), std::abs(dz)});
    };
    const auto checkLine = [&context, &greatestDistance](
        int fromX, int fromY, int fromZ,
        int toX, int toY, int toZ)
    {
        const int dx = toX - fromX;
        const int dy = toY - fromY;
        const int dz = toZ - fromZ;
        const int distance = greatestDistance(dx, dy, dz);
        if (distance == 0)
            return -1;

        const float stepX = static_cast<float>(dx) / distance;
        const float stepY = static_cast<float>(dy) / distance;
        const float stepZ = static_cast<float>(dz) / distance;
        for (int step = 0; step <= distance; ++step)
        {
            const int checkX = fromX + static_cast<int>(std::floor(
                0.5F + static_cast<float>(step) * stepX
            ));
            const int checkY = fromY + static_cast<int>(std::floor(
                0.5F + static_cast<float>(step) * stepY
            ));
            const int checkZ = fromZ + static_cast<int>(std::floor(
                0.5F + static_cast<float>(step) * stepZ
            ));
            if (!canReplace(context.getBlock(checkX, checkY, checkZ)))
                return step;
        }
        return -1;
    };

    const int obstruction = checkLine(
        x, y, z, x, y + heightLimit - 1, z
    );
    if (obstruction != -1)
    {
        if (obstruction < 6)
            return false;
        heightLimit = obstruction;
    }
    if (y + heightLimit + 1 > Chunk::HEIGHT)
        return false;

    int trunkHeight = static_cast<int>(heightLimit * 0.618);
    if (trunkHeight >= heightLimit)
        trunkHeight = heightLimit - 1;

    struct LeafNode
    {
        int x;
        int y;
        int z;
        int branchY;
    };
    constexpr int leafDistance = 5;
    int nodesPerLayer = static_cast<int>(
        1.382 + std::pow(static_cast<double>(heightLimit) / 13.0, 2.0)
    );
    nodesPerLayer = std::max(nodesPerLayer, 1);
    const int trunkTopY = y + trunkHeight;
    int layer = heightLimit - leafDistance;
    std::vector<LeafNode> nodes;
    nodes.push_back({x, y + layer, z, trunkTopY});

    const auto layerSize = [heightLimit](int layerY)
    {
        if (static_cast<float>(layerY) < heightLimit * 0.3F)
            return -1.0F;
        const float half = static_cast<float>(heightLimit) / 2.0F;
        const float fromMiddle = half - static_cast<float>(layerY);
        if (fromMiddle == 0.0F)
            return half * 0.5F;
        if (std::abs(fromMiddle) >= half)
            return 0.0F;
        return std::sqrt(half * half - fromMiddle * fromMiddle) * 0.5F;
    };

    for (; layer >= 0; --layer)
    {
        const float radius = layerSize(layer);
        if (radius < 0.0F)
            continue;

        for (int attempt = 0; attempt < nodesPerLayer; ++attempt)
        {
            const double distance = static_cast<double>(radius) *
                (static_cast<double>(treeRandom.nextFloat()) + 0.328);
            const double angle = static_cast<double>(treeRandom.nextFloat()) *
                std::numbers::pi * 2.0;
            const int nodeX = static_cast<int>(std::floor(
                static_cast<double>(x) + distance * std::sin(angle) + 0.5
            ));
            const int nodeY = y + layer - 1;
            const int nodeZ = static_cast<int>(std::floor(
                static_cast<double>(z) + distance * std::cos(angle) + 0.5
            ));
            if (checkLine(
                    nodeX, nodeY, nodeZ,
                    nodeX, nodeY + leafDistance, nodeZ) != -1)
                continue;

            const int deltaX = x - nodeX;
            const int deltaZ = z - nodeZ;
            const double branchBase = static_cast<double>(nodeY) -
                std::sqrt(static_cast<double>(
                    deltaX * deltaX + deltaZ * deltaZ
                )) * 0.381;
            const int branchY = branchBase > trunkTopY
                ? trunkTopY
                : static_cast<int>(branchBase);
            if (checkLine(x, branchY, z, nodeX, nodeY, nodeZ) == -1)
                nodes.push_back({nodeX, nodeY, nodeZ, branchY});
        }
    }

    for (const LeafNode& node : nodes)
    {
        for (int leafLayer = 0; leafLayer < leafDistance; ++leafLayer)
        {
            const float leafRadius =
                (leafLayer == 0 || leafLayer == leafDistance - 1)
                ? 2.0F
                : 3.0F;
            const int extent = static_cast<int>(leafRadius + 0.618F);
            for (int dx = -extent; dx <= extent; ++dx)
            {
                for (int dz = -extent; dz <= extent; ++dz)
                {
                    const double sampleX = std::abs(dx) + 0.5;
                    const double sampleZ = std::abs(dz) + 0.5;
                    if (sampleX * sampleX + sampleZ * sampleZ <=
                        static_cast<double>(leafRadius * leafRadius))
                    {
                        placeLeaf(
                            context,
                            node.x + dx,
                            node.y + leafLayer,
                            node.z + dz,
                            BlockType::OakLeaves
                        );
                    }
                }
            }
        }
    }

    const auto placeLimb = [&context, &greatestDistance](
        int fromX, int fromY, int fromZ,
        int toX, int toY, int toZ)
    {
        const int dx = toX - fromX;
        const int dy = toY - fromY;
        const int dz = toZ - fromZ;
        const int distance = greatestDistance(dx, dy, dz);
        if (distance == 0)
        {
            context.setBlock(fromX, fromY, fromZ, BlockType::OakLog);
            return;
        }
        const float stepX = static_cast<float>(dx) / distance;
        const float stepY = static_cast<float>(dy) / distance;
        const float stepZ = static_cast<float>(dz) / distance;
        for (int step = 0; step <= distance; ++step)
        {
            context.setBlock(
                fromX + static_cast<int>(std::floor(
                    0.5F + static_cast<float>(step) * stepX
                )),
                fromY + static_cast<int>(std::floor(
                    0.5F + static_cast<float>(step) * stepY
                )),
                fromZ + static_cast<int>(std::floor(
                    0.5F + static_cast<float>(step) * stepZ
                )),
                BlockType::OakLog
            );
        }
    };

    context.setBlock(x, y - 1, z, BlockType::Dirt);
    placeLimb(x, y, z, x, trunkTopY, z);
    for (const LeafNode& node : nodes)
    {
        if (static_cast<double>(node.branchY - y) >= heightLimit * 0.2)
            placeLimb(x, node.branchY, z, node.x, node.y, node.z);
    }
    return true;
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

                if ((!corner ||
                     (random.nextInt(2) != 0 && layer != 0)) &&
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
                if ((!corner ||
                     (random.nextInt(2) != 0 && layer != 0)) &&
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
    // BiomeJungle first chooses a 4..10 minimum, then WorldGenTrees adds
    // another 0..2 blocks to the final height.
    const int minimumHeight = 4 + random.nextInt(7);
    const int height = minimumHeight + random.nextInt(3);
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
                    std::abs(dx) == radius && std::abs(dz) == radius;
                if ((!corner ||
                     (random.nextInt(2) != 0 && layer != 0)) &&
                    canReplace(context.getBlock(xx, yy, zz)))
                {
                    context.setBlock(xx, yy, zz, BlockType::JungleLeaves);
                }
            }
        }
    }

    for (int level = 0; level < height; ++level)
    {
        if (canReplace(context.getBlock(x, y + level, z)))
            context.setBlock(x, y + level, z, BlockType::JungleLog);
    }
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
                if ((!corner || random.nextInt(2) != 0) &&
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
    const int height = 10 + random.nextInt(3) + random.nextInt(20);
    if (y < 1 || y + height + 1 > Chunk::HEIGHT)
        return false;

    // WorldGenHugeTrees validates a radius-two column. The old radius-four
    // top check rejected border replays asymmetrically and left partial
    // canopies behind.
    for (int yy = y; yy <= y + height + 1; ++yy)
    {
        const int radius = yy == y ? 1 : 2;
        for (int xx = x - radius; xx <= x + radius; ++xx)
            for (int zz = z - radius; zz <= z + radius; ++zz)
                if (!canOccupy(context, xx, yy, zz))
                    return false;
    }

    if (y < 2 || !hasValidSoil(context, x, y, z))
        return false;

    for (int dx = 0; dx < 2; ++dx)
        for (int dz = 0; dz < 2; ++dz)
            context.setBlock(x + dx, y - 1, z + dz, BlockType::Dirt);

    // WorldGenMegaJungle.createCrown uses three strict, 2x2-centred layers.
    for (int offsetY = -2; offsetY <= 0; ++offsetY)
    {
        growStrictLeafLayer(
            context,
            x,
            y + height + offsetY,
            z,
            3 - offsetY,
            BlockType::JungleLeaves
        );
    }

    // Large jungle trees carry descending lateral limbs from the upper half.
    for (int branchY = y + height - 2 - random.nextInt(4);
         branchY > y + height / 2;
         branchY -= 2 + random.nextInt(4))
    {
        const float angle = random.nextFloat() *
            static_cast<float>(std::numbers::pi * 2.0);
        int branchX = x;
        int branchZ = z;
        for (int step = 0; step < 5; ++step)
        {
            branchX = x + static_cast<int>(
                1.5F + std::cos(angle) * static_cast<float>(step)
            );
            branchZ = z + static_cast<int>(
                1.5F + std::sin(angle) * static_cast<float>(step)
            );
            context.setBlock(
                branchX,
                branchY - 3 + step / 2,
                branchZ,
                BlockType::JungleLog
            );
        }

        const int lowerLeaves = 1 + random.nextInt(2);
        for (int leafY = branchY - lowerLeaves;
             leafY <= branchY;
             ++leafY)
        {
            growLeafCircle(
                context,
                branchX,
                leafY,
                branchZ,
                1 - (leafY - branchY),
                BlockType::JungleLeaves
            );
        }
    }

    for (int level = 0; level < height; ++level)
    {
        if (canReplace(context.getBlock(x, y + level, z)))
            context.setBlock(x, y + level, z, BlockType::JungleLog);

        if (level >= height - 1)
            continue;
        for (const auto& offset : std::array<std::array<int, 2>, 3>{
                 std::array<int, 2>{1, 0},
                 std::array<int, 2>{1, 1},
                 std::array<int, 2>{0, 1}})
        {
            const int logX = x + offset[0];
            const int logZ = z + offset[1];
            if (canReplace(context.getBlock(logX, y + level, logZ)))
            {
                context.setBlock(
                    logX, y + level, logZ, BlockType::JungleLog
                );
            }
        }
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
            if (std::abs(dx) != 3 || std::abs(dz) != 3)
                placeLeaf(
                    context,
                    trunkX + dx,
                    canopyY,
                    trunkZ + dz,
                    BlockType::AcaciaLeaves
                );
    }
    for (int dx = -1; dx <= 1; ++dx)
        for (int dz = -1; dz <= 1; ++dz)
            placeLeaf(
                context,
                trunkX + dx,
                canopyY + 1,
                trunkZ + dz,
                BlockType::AcaciaLeaves
            );
    constexpr int extended[4][2] = {{2,0},{-2,0},{0,2},{0,-2}};
    for (const auto& offset : extended)
        placeLeaf(
            context,
            trunkX + offset[0],
            canopyY + 1,
            trunkZ + offset[1],
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
                    if (std::abs(dx) != 2 || std::abs(dz) != 2)
                        placeLeaf(
                            context,
                            branchX + dx,
                            branchY,
                            branchZ + dz,
                            BlockType::AcaciaLeaves
                        );
            for (int dx = -1; dx <= 1; ++dx)
                for (int dz = -1; dz <= 1; ++dz)
                    placeLeaf(
                        context,
                        branchX + dx,
                        branchY + 1,
                        branchZ + dz,
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
    const int height = 6 + random.nextInt(3) + random.nextInt(2);
    if (y < 1 || y + height + 1 >= Chunk::HEIGHT)
        return false;

    for (int level = 0; level <= height + 1; ++level)
    {
        int radius = 1;
        if (level == 0)
            radius = 0;
        else if (level >= height - 1)
            radius = 2;

        for (int dx = -radius; dx <= radius; ++dx)
            for (int dz = -radius; dz <= radius; ++dz)
                if (!canOccupy(context, x + dx, y + level, z + dz))
                    return false;
    }

    if (!hasValidSoil(context, x, y, z))
        return false;

    for (int dx = 0; dx < 2; ++dx)
        for (int dz = 0; dz < 2; ++dz)
            context.setBlock(x + dx, y - 1, z + dz, BlockType::Dirt);

    constexpr std::array<std::array<int, 2>, 4> directions{{
        {{1, 0}}, {{-1, 0}}, {{0, 1}}, {{0, -1}}
    }};
    const auto& direction = directions[
        static_cast<std::size_t>(random.nextInt(4))
    ];
    const int bendStart = height - random.nextInt(4);
    int bendLength = 2 - random.nextInt(3);
    int trunkX = x;
    int trunkZ = z;
    const int canopyY = y + height - 1;

    for (int level = 0; level < height; ++level)
    {
        if (level >= bendStart && bendLength > 0)
        {
            trunkX += direction[0];
            trunkZ += direction[1];
            --bendLength;
        }

        const int trunkY = y + level;
        const BlockType current = context.getBlock(trunkX, trunkY, trunkZ);
        if (current == BlockType::Air || isLeaf(current))
        {
            for (int dx = 0; dx < 2; ++dx)
                for (int dz = 0; dz < 2; ++dz)
                    if (canReplace(context.getBlock(
                            trunkX + dx, trunkY, trunkZ + dz)))
                    {
                        context.setBlock(
                            trunkX + dx,
                            trunkY,
                            trunkZ + dz,
                            BlockType::DarkOakLog
                        );
                    }
        }
    }

    const auto placeDarkLeaf = [&context](int leafX, int leafY, int leafZ)
    {
        if (context.getBlock(leafX, leafY, leafZ) == BlockType::Air)
        {
            context.setBlock(
                leafX, leafY, leafZ, BlockType::DarkOakLeaves
            );
        }
    };

    for (int dx = -2; dx <= 0; ++dx)
    {
        for (int dz = -2; dz <= 0; ++dz)
        {
            placeDarkLeaf(trunkX + dx, canopyY - 1, trunkZ + dz);
            placeDarkLeaf(1 + trunkX - dx, canopyY - 1, trunkZ + dz);
            placeDarkLeaf(trunkX + dx, canopyY - 1, 1 + trunkZ - dz);
            placeDarkLeaf(
                1 + trunkX - dx, canopyY - 1, 1 + trunkZ - dz
            );

            if ((dx > -2 || dz > -1) && (dx != -1 || dz != -2))
            {
                placeDarkLeaf(trunkX + dx, canopyY + 1, trunkZ + dz);
                placeDarkLeaf(1 + trunkX - dx, canopyY + 1, trunkZ + dz);
                placeDarkLeaf(trunkX + dx, canopyY + 1, 1 + trunkZ - dz);
                placeDarkLeaf(
                    1 + trunkX - dx, canopyY + 1, 1 + trunkZ - dz
                );
            }
        }
    }

    if (random.nextBoolean())
    {
        placeDarkLeaf(trunkX, canopyY + 2, trunkZ);
        placeDarkLeaf(trunkX + 1, canopyY + 2, trunkZ);
        placeDarkLeaf(trunkX + 1, canopyY + 2, trunkZ + 1);
        placeDarkLeaf(trunkX, canopyY + 2, trunkZ + 1);
    }

    for (int dx = -3; dx <= 4; ++dx)
    {
        for (int dz = -3; dz <= 4; ++dz)
        {
            const bool corner =
                (dx == -3 || dx == 4) && (dz == -3 || dz == 4);
            if (!corner && (std::abs(dx) < 3 || std::abs(dz) < 3))
                placeDarkLeaf(trunkX + dx, canopyY, trunkZ + dz);
        }
    }

    for (int branchX = -1; branchX <= 2; ++branchX)
    {
        for (int branchZ = -1; branchZ <= 2; ++branchZ)
        {
            const bool outsideTrunk = branchX < 0 || branchX > 1 ||
                branchZ < 0 || branchZ > 1;
            if (!outsideTrunk || random.nextInt(3) > 0)
                continue;

            const int length = random.nextInt(3) + 2;
            for (int segment = 0; segment < length; ++segment)
            {
                const int logY = canopyY - segment - 1;
                if (canReplace(context.getBlock(
                        x + branchX, logY, z + branchZ)))
                {
                    context.setBlock(
                        x + branchX,
                        logY,
                        z + branchZ,
                        BlockType::DarkOakLog
                    );
                }
            }

            for (int dx = -1; dx <= 1; ++dx)
                for (int dz = -1; dz <= 1; ++dz)
                    placeDarkLeaf(
                        trunkX + branchX + dx,
                        canopyY,
                        trunkZ + branchZ + dz
                    );

            for (int dx = -2; dx <= 2; ++dx)
            {
                for (int dz = -2; dz <= 2; ++dz)
                {
                    if (std::abs(dx) != 2 || std::abs(dz) != 2)
                    {
                        placeDarkLeaf(
                            trunkX + branchX + dx,
                            canopyY - 1,
                            trunkZ + branchZ + dz
                        );
                    }
                }
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
    bool tallCrown) const
{
    const int height = 13 + random.nextInt(3) + random.nextInt(15);
    if (y < 1 || y + height + 1 > Chunk::HEIGHT)
        return false;

    for (int level = 0; level <= height + 1; ++level)
    {
        const int radius = level == 0 ? 1 : 2;
        for (int dx = -radius; dx <= radius; ++dx)
            for (int dz = -radius; dz <= radius; ++dz)
                if (!canOccupy(context, x + dx, y + level, z + dz))
                    return false;
    }
    if (y < 2 || !hasValidSoil(context, x, y, z))
        return false;

    for (int dx = 0; dx < 2; ++dx)
        for (int dz = 0; dz < 2; ++dz)
            context.setBlock(x + dx, y - 1, z + dz, BlockType::Dirt);

    const int crownHeight = random.nextInt(5) + (tallCrown ? 13 : 3);
    int previousRadius = -1;
    for (int yy = y + height - crownHeight; yy <= y + height; ++yy)
    {
        const int distanceFromTop = y + height - yy;
        int radius = static_cast<int>(std::floor(
            static_cast<float>(distanceFromTop) /
            static_cast<float>(crownHeight) * 3.5f
        ));
        if (distanceFromTop > 0 && radius == previousRadius &&
            (yy & 1) == 0)
        {
            ++radius;
        }
        growStrictLeafLayer(
            context, x, yy, z, radius, BlockType::SpruceLeaves
        );
        previousRadius = radius;
    }

    for (int level = 0; level < height; ++level)
    {
        if (canReplace(context.getBlock(x, y + level, z)))
            context.setBlock(x, y + level, z, BlockType::SpruceLog);

        if (level >= height - 1)
            continue;
        for (const auto& offset : std::array<std::array<int, 2>, 3>{
                 std::array<int, 2>{1, 0},
                 std::array<int, 2>{1, 1},
                 std::array<int, 2>{0, 1}})
        {
            if (canReplace(context.getBlock(
                    x + offset[0], y + level, z + offset[1])))
            {
                context.setBlock(
                    x + offset[0],
                    y + level,
                    z + offset[1],
                    BlockType::SpruceLog
                );
            }
        }
    }

    // WorldGenMegaPineTree.generateSaplings spreads podzol in four circles
    // around the 2x2 trunk. Apply the deterministic core patches here; the
    // clone does not yet have a separate sapling post-pass.
    constexpr int patchCentres[4][2] = {
        {-1, -1}, {2, -1}, {-1, 2}, {2, 2}
    };
    const auto placePodzolPatch = [&context, x, y, z](
        int centerX,
        int centerZ)
    {
        for (int dx = -2; dx <= 2; ++dx)
        {
            for (int dz = -2; dz <= 2; ++dz)
            {
                if (std::abs(dx) == 2 && std::abs(dz) == 2)
                    continue;
                const int patchX = x + centerX + dx;
                const int patchZ = z + centerZ + dz;
                for (int offsetY = 2; offsetY >= -3; --offsetY)
                {
                    const int patchY = y - 1 + offsetY;
                    const BlockType block = context.getBlock(
                        patchX, patchY, patchZ
                    );
                    if (block == BlockType::Grass || block == BlockType::Dirt)
                    {
                        context.setBlock(
                            patchX, patchY, patchZ, BlockType::Podzol
                        );
                        break;
                    }
                    if (block != BlockType::Air && offsetY < 0)
                        break;
                }
            }
        }
    };

    for (const auto& centre : patchCentres)
        placePodzolPatch(centre[0], centre[1]);

    for (int attempt = 0; attempt < 5; ++attempt)
    {
        const int point = random.nextInt(64);
        const int offsetX = point % 8;
        const int offsetZ = point / 8;
        if (offsetX == 0 || offsetX == 7 ||
            offsetZ == 0 || offsetZ == 7)
        {
            placePodzolPatch(-3 + offsetX, -3 + offsetZ);
        }
    }
    return true;
}
