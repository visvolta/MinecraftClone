#pragma once

#include "worldgen/StructurePrimitives.h"

#include <cstdint>
#include <memory>
#include <unordered_set>
#include <vector>

class WorldGenerationContext;

namespace mc112
{
class OceanMonumentStructure
{
public:
    struct Start
    {
        std::vector<std::unique_ptr<Piece>> pieces;
        Box bounds;
        bool sizeable = false;
        std::unordered_set<std::uint64_t> processedChunks;
    };

    [[nodiscard]] static Start create(
        std::int64_t worldSeed,
        int chunkX,
        int chunkZ,
        JavaRandom& mapGenRandom);

    static void place(
        Start& start,
        WorldGenerationContext& context,
        JavaRandom& populationRandom,
        int sourceChunkX,
        int sourceChunkZ,
        const Box& clip);
};
}
