#pragma once

class JavaRandom;
class WorldGenerationContext;

class OreGenerator
{
public:
    void generate(
        WorldGenerationContext& context,
        JavaRandom& random,
        int sourceChunkOriginX,
        int sourceChunkOriginZ) const;
};
