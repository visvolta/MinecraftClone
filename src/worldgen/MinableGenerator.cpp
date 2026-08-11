#include "worldgen/MinableGenerator.h"

#include "worldgen/JavaRandom.h"
#include "worldgen/WorldGenerationContext.h"
#include "worldgen/Vanilla112State.h"

#include <algorithm>
#include <cmath>
#include <numbers>

MinableGenerator::MinableGenerator(BlockType generatedBlock, int veinSize)
    : MinableGenerator(mc::content::BlockState(generatedBlock), veinSize)
{
}

MinableGenerator::MinableGenerator(
    mc::content::BlockState generatedState,
    int veinSize)
    : generatedState_(generatedState), veinSize_(std::max(1, veinSize))
{
}

void MinableGenerator::generate(
    WorldGenerationContext& context,
    JavaRandom& random,
    int worldX,
    int worldY,
    int worldZ) const
{
    // Exact WorldGenMinable 1.12.2 geometry and Random call order.
    const float angle = random.nextFloat() * std::numbers::pi_v<float>;
    const double startX = static_cast<double>(
        static_cast<float>(worldX + 8) +
        std::sin(angle) * static_cast<float>(veinSize_) / 8.0F);
    const double endX = static_cast<double>(
        static_cast<float>(worldX + 8) -
        std::sin(angle) * static_cast<float>(veinSize_) / 8.0F);
    const double startZ = static_cast<double>(
        static_cast<float>(worldZ + 8) +
        std::cos(angle) * static_cast<float>(veinSize_) / 8.0F);
    const double endZ = static_cast<double>(
        static_cast<float>(worldZ + 8) -
        std::cos(angle) * static_cast<float>(veinSize_) / 8.0F);
    const double startY = static_cast<double>(worldY + random.nextInt(3) - 2);
    const double endY = static_cast<double>(worldY + random.nextInt(3) - 2);

    for (int step = 0; step < veinSize_; ++step)
    {
        const float progress = static_cast<float>(step) /
            static_cast<float>(veinSize_);
        const double centerX = startX + (endX - startX) * progress;
        const double centerY = startY + (endY - startY) * progress;
        const double centerZ = startZ + (endZ - startZ) * progress;
        const double randomScale = random.nextDouble() *
            static_cast<double>(veinSize_) / 16.0;
        const double diameter = static_cast<double>(
            std::sin(std::numbers::pi_v<float> * progress) + 1.0F) *
            randomScale + 1.0;

        const int minX = static_cast<int>(std::floor(centerX - diameter / 2.0));
        const int minY = static_cast<int>(std::floor(centerY - diameter / 2.0));
        const int minZ = static_cast<int>(std::floor(centerZ - diameter / 2.0));
        const int maxX = static_cast<int>(std::floor(centerX + diameter / 2.0));
        const int maxY = static_cast<int>(std::floor(centerY + diameter / 2.0));
        const int maxZ = static_cast<int>(std::floor(centerZ + diameter / 2.0));

        for (int x = minX; x <= maxX; ++x)
        {
            const double nx = (static_cast<double>(x) + 0.5 - centerX) /
                (diameter / 2.0);
            if (nx * nx >= 1.0) continue;
            for (int y = minY; y <= maxY; ++y)
            {
                const double ny = (static_cast<double>(y) + 0.5 - centerY) /
                    (diameter / 2.0);
                if (nx * nx + ny * ny >= 1.0) continue;
                for (int z = minZ; z <= maxZ; ++z)
                {
                    const double nz = (static_cast<double>(z) + 0.5 - centerZ) /
                        (diameter / 2.0);
                    if (nx * nx + ny * ny + nz * nz >= 1.0)
                        continue;
                    // BlockStone.EnumType::func_190912_e() is true only for
                    // STONE, GRANITE, DIORITE and ANDESITE (not polished
                    // variants). Preserve that exact 1.12.2 predicate even
                    // when those variants are resource-backed states.
                    if (mc112::isNaturalStone(context.getBlockState(x, y, z)))
                        context.setBlockState(x, y, z, generatedState_);
                }
            }
        }
    }
}
