#pragma once

#include "worldgen/StructurePrimitives.h"
#include "worldgen/Biome.h"

#include <memory>
#include <vector>

class WorldGenerationContext;

namespace mc112
{
class VillageStructure
{
public:
    struct Start
    {
        std::vector<std::unique_ptr<Piece>> pieces;
        Box bounds;
        bool sizeable=false;
        int structureType=0;
        bool zombie=false;
    };

    [[nodiscard]] static Start create(
        int chunkX,int chunkZ,BiomeId biome,JavaRandom& random,int terrainType=0);
    static void place(
        Start& start,WorldGenerationContext& context,JavaRandom& random,const Box& clip);
};
}
