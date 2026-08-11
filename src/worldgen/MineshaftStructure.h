#pragma once

#include "worldgen/StructurePrimitives.h"

#include <cstdint>
#include <memory>
#include <vector>

class WorldGenerationContext;

namespace mc112
{
class MineshaftStructure
{
public:
    enum class Type : std::uint8_t { Normal, Mesa };

    struct Start
    {
        std::vector<std::unique_ptr<Piece>> pieces;
        Box bounds;
        bool sizeable = false;
    };

    [[nodiscard]] static Start create(
        int chunkX,
        int chunkZ,
        Type type,
        JavaRandom& random,
        int seaLevel = 63);

    static void place(
        Start& start,
        WorldGenerationContext& context,
        JavaRandom& random,
        const Box& chunkClip);
};
}
