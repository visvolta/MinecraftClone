#pragma once

#include "worldgen/StructurePrimitives.h"

#include <memory>
#include <vector>

class WorldGenerationContext;

namespace mc112
{
class StrongholdStructure
{
public:
    struct Start
    {
        std::vector<std::unique_ptr<Piece>> pieces;
        Box bounds;
        bool sizeable=false;
        bool hasPortalRoom=false;
    };

    [[nodiscard]] static Start create(int chunkX,int chunkZ,JavaRandom& random,int seaLevel=63);
    static void place(Start&,WorldGenerationContext&,JavaRandom&,const Box& clip);
};
}
