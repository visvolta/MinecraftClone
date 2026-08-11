#pragma once

#include "worldgen/StructureTemplate.h"

#include <cstdint>

class WorldGenerationContext;

namespace mc112
{
// Literal port of WorldGenFossils from Java 1.12.2. The population Random is
// only used by BiomeSwamp for the 1-in-64 gate; the fossil generator itself
// creates Chunk::getRandomWithSeed(987234911L), exactly like vanilla.
class FossilGenerator
{
public:
    explicit FossilGenerator(std::int64_t worldSeed);

    bool generate(
        WorldGenerationContext& context,
        int sourceChunkX,
        int sourceChunkZ) const;

private:
    std::int64_t worldSeed_ = 0;
    mutable StructureTemplateLibrary templates_;
};
}
