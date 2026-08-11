#include "worldgen/MinableGenerator.h"

#include "worldgen/JavaRandom.h"
#include "worldgen/WorldGenerationContext.h"

#include <algorithm>
#include <cmath>
#include <numbers>

MinableGenerator::MinableGenerator(BlockType generatedBlock, int veinSize)
    : generatedBlock_(generatedBlock), veinSize_(std::max(1, veinSize)) {}

void MinableGenerator::generate(
    WorldGenerationContext& context,
    JavaRandom& random,
    int worldX,
    int worldY,
    int worldZ) const
{
    const float angle = random.nextFloat() * static_cast<float>(std::numbers::pi);
    const double startX = static_cast<double>(worldX + 8) +
        std::sin(angle) * static_cast<double>(veinSize_) / 8.0;
    const double endX = static_cast<double>(worldX + 8) -
        std::sin(angle) * static_cast<double>(veinSize_) / 8.0;
    const double startZ = static_cast<double>(worldZ + 8) +
        std::cos(angle) * static_cast<double>(veinSize_) / 8.0;
    const double endZ = static_cast<double>(worldZ + 8) -
        std::cos(angle) * static_cast<double>(veinSize_) / 8.0;
    const double startY = static_cast<double>(worldY + random.nextInt(3) - 2);
    const double endY = static_cast<double>(worldY + random.nextInt(3) - 2);

    // WorldGenMinable uses i < numberOfBlocks. The old clone used <= and
    // therefore consumed one extra double and produced a different vein.
    for (int step = 0; step < veinSize_; ++step)
    {
        const float progress = static_cast<float>(step) /
                               static_cast<float>(veinSize_);
        const double centreX = startX + (endX - startX) * progress;
        const double centreY = startY + (endY - startY) * progress;
        const double centreZ = startZ + (endZ - startZ) * progress;
        const double randomScale = random.nextDouble() *
                                   static_cast<double>(veinSize_) / 16.0;
        const double diameter =
            (std::sin(static_cast<float>(std::numbers::pi) * progress) + 1.0) *
                randomScale + 1.0;

        const int minX = static_cast<int>(std::floor(centreX - diameter / 2.0));
        const int maxX = static_cast<int>(std::floor(centreX + diameter / 2.0));
        const int minY = static_cast<int>(std::floor(centreY - diameter / 2.0));
        const int maxY = static_cast<int>(std::floor(centreY + diameter / 2.0));
        const int minZ = static_cast<int>(std::floor(centreZ - diameter / 2.0));
        const int maxZ = static_cast<int>(std::floor(centreZ + diameter / 2.0));

        for (int x = minX; x <= maxX; ++x)
        {
            const double nx = (static_cast<double>(x) + 0.5 - centreX) /
                              (diameter / 2.0);
            if (nx * nx >= 1.0)
                continue;
            for (int y = minY; y <= maxY; ++y)
            {
                const double ny = (static_cast<double>(y) + 0.5 - centreY) /
                                  (diameter / 2.0);
                if (nx * nx + ny * ny >= 1.0)
                    continue;
                for (int z = minZ; z <= maxZ; ++z)
                {
                    const double nz = (static_cast<double>(z) + 0.5 - centreZ) /
                                      (diameter / 2.0);
                    if (nx * nx + ny * ny + nz * nz < 1.0 &&
                        context.getBlock(x, y, z) == BlockType::Stone)
                        context.setBlock(x, y, z, generatedBlock_);
                }
            }
        }
    }
}
