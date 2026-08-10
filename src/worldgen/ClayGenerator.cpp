#include "worldgen/ClayGenerator.h"

#include "Block.h"
#include "worldgen/JavaRandom.h"
#include "worldgen/WorldGenerationContext.h"

#include <algorithm>
#include <cmath>
#include <numbers>

ClayGenerator::ClayGenerator(int depositSize)
    : depositSize_(std::max(1, depositSize))
{
}

bool ClayGenerator::generate(
    WorldGenerationContext& context,
    JavaRandom& random,
    int worldX,
    int worldY,
    int worldZ) const
{
    if (!isLiquid(context.getBlock(worldX, worldY, worldZ)))
    {
        return false;
    }

    const float angle =
        random.nextFloat() * static_cast<float>(std::numbers::pi);

    const double startX =
        static_cast<double>(worldX + 8) +
        std::sin(angle) * static_cast<double>(depositSize_) / 8.0;
    const double endX =
        static_cast<double>(worldX + 8) -
        std::sin(angle) * static_cast<double>(depositSize_) / 8.0;

    const double startZ =
        static_cast<double>(worldZ + 8) +
        std::cos(angle) * static_cast<double>(depositSize_) / 8.0;
    const double endZ =
        static_cast<double>(worldZ + 8) -
        std::cos(angle) * static_cast<double>(depositSize_) / 8.0;

    const double startY =
        static_cast<double>(worldY + random.nextInt(3) + 2);
    const double endY =
        static_cast<double>(worldY + random.nextInt(3) + 2);

    for (int step = 0; step <= depositSize_; ++step)
    {
        const double progress =
            static_cast<double>(step) / static_cast<double>(depositSize_);

        const double centreX = startX + (endX - startX) * progress;
        const double centreY = startY + (endY - startY) * progress;
        const double centreZ = startZ + (endZ - startZ) * progress;

        const double randomScale =
            random.nextDouble() * static_cast<double>(depositSize_) / 16.0;

        const double horizontalDiameter =
            (std::sin(
                 static_cast<double>(step) *
                 std::numbers::pi /
                 static_cast<double>(depositSize_)) +
             1.0) *
                randomScale +
            1.0;

        const double verticalDiameter = horizontalDiameter;

        const int minX =
            static_cast<int>(std::floor(centreX - horizontalDiameter / 2.0));
        const int maxX =
            static_cast<int>(std::floor(centreX + horizontalDiameter / 2.0));
        const int minY =
            static_cast<int>(std::floor(centreY - verticalDiameter / 2.0));
        const int maxY =
            static_cast<int>(std::floor(centreY + verticalDiameter / 2.0));
        const int minZ =
            static_cast<int>(std::floor(centreZ - horizontalDiameter / 2.0));
        const int maxZ =
            static_cast<int>(std::floor(centreZ + horizontalDiameter / 2.0));

        for (int x = minX; x <= maxX; ++x)
        {
            for (int y = minY; y <= maxY; ++y)
            {
                for (int z = minZ; z <= maxZ; ++z)
                {
                    const double normalizedX =
                        (static_cast<double>(x) + 0.5 - centreX) /
                        (horizontalDiameter / 2.0);
                    const double normalizedY =
                        (static_cast<double>(y) + 0.5 - centreY) /
                        (verticalDiameter / 2.0);
                    const double normalizedZ =
                        (static_cast<double>(z) + 0.5 - centreZ) /
                        (horizontalDiameter / 2.0);

                    if (normalizedX * normalizedX +
                                normalizedY * normalizedY +
                                normalizedZ * normalizedZ <
                            1.0 &&
                        context.getBlock(x, y, z) == BlockType::Sand)
                    {
                        context.setBlock(x, y, z, BlockType::Clay);
                    }
                }
            }
        }
    }

    return true;
}
