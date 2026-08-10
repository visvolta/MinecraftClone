#pragma once

#include <cstdint>

class Chunk;
class WorldGenerationContext;

class PopulationGenerator
{
public:
    explicit PopulationGenerator(std::int64_t worldSeed);

    void populate(
        Chunk& targetChunk,
        WorldGenerationContext& context) const;

private:
    std::int64_t worldSeed_ = 0;
};
