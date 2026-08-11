#pragma once

#include "worldgen/StructurePrimitives.h"
#include "worldgen/StructureTemplate.h"

#include <cstdint>
#include <string>
#include <vector>

class WorldGenerationContext;

namespace mc112
{
class WoodlandMansionStructure
{
public:
    struct TemplatePiece
    {
        std::string name;
        int x=0,y=0,z=0;
        Rotation rotation=Rotation::None;
        Mirror mirror=Mirror::None;
    };

    struct Start
    {
        std::vector<TemplatePiece> pieces;
        Box bounds;
        int minY=0;
        bool sizeable=false;
    };

    [[nodiscard]] static Start create(
        int chunkX,int chunkZ,JavaRandom& random,
        const WorldGenerationContext& context);

    static void place(
        Start& start,WorldGenerationContext& context,
        JavaRandom& populationRandom,const Box& clip);
};
}
