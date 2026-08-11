#include "worldgen/ClayGenerator.h"

#include "Block.h"
#include "worldgen/JavaRandom.h"
#include "worldgen/WorldGenerationContext.h"

#include <algorithm>

ClayGenerator::ClayGenerator(int depositSize)
    : depositSize_(std::max(3, depositSize))
{
}

bool ClayGenerator::generate(
    WorldGenerationContext& context,
    JavaRandom& random,
    int worldX,
    int worldY,
    int worldZ) const
{
    // WorldGenClay (1.12.2): the selected top-solid-or-liquid position must
    // actually be water. The generator is a shallow circular dirt/clay patch,
    // not an ore-style interpolated ellipsoid.
    if (context.getBlock(worldX, worldY, worldZ) != BlockType::Water)
        return false;

    const int radius = random.nextInt(depositSize_ - 2) + 2;
    for (int x = worldX - radius; x <= worldX + radius; ++x)
    {
        for (int z = worldZ - radius; z <= worldZ + radius; ++z)
        {
            const int dx = x - worldX;
            const int dz = z - worldZ;
            if (dx * dx + dz * dz > radius * radius)
                continue;

            for (int y = worldY - 1; y <= worldY + 1; ++y)
            {
                const BlockType current = context.getBlock(x, y, z);
                if (current == BlockType::Dirt || current == BlockType::Clay)
                    context.setBlock(x, y, z, BlockType::Clay);
            }
        }
    }
    return true;
}
