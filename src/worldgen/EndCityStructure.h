#pragma once

#include "worldgen/StructurePrimitives.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

class WorldGenerationContext;

namespace mc112::EndCityStructure
{
using HeightSampler = std::function<int(int,int)>;
using IslandPredicate = std::function<bool(int,int)>;

struct Start
{
    int chunkX=0, chunkZ=0;
    bool sizeable=false;
    std::vector<std::unique_ptr<Piece>> pieces;
    Box bounds;
    bool place(WorldGenerationContext&,JavaRandom&,const Box&) const;
};

[[nodiscard]] bool isCandidate(std::int64_t worldSeed,int chunkX,int chunkZ,const IslandPredicate& isIsland);
[[nodiscard]] Start create(std::int64_t worldSeed,int chunkX,int chunkZ,JavaRandom& populationRandom,const HeightSampler& height);
}
