#pragma once

#include "worldgen/Biome.h"
#include "worldgen/JavaRandom.h"
#include "worldgen/StructurePrimitives.h"
#include "worldgen/StructureTemplate.h"

#include <cstdint>
#include <memory>

class WorldGenerationContext;

namespace mc112
{
class ScatteredFeatureStructure
{
public:
    enum class Kind : std::uint8_t
    {
        None,
        DesertPyramid,
        JunglePyramid,
        SwampHut,
        Igloo
    };

    struct Start
    {
        Kind kind = Kind::None;
        std::unique_ptr<Piece> piece;
        Box bounds;
        bool sizeable = false;
    };

    [[nodiscard]] static Start create(
        int chunkX,
        int chunkZ,
        BiomeId biome,
        JavaRandom& random);

    static void place(
        Start& start,
        WorldGenerationContext& context,
        JavaRandom& populationRandom,
        const Box& clip);
};
}
