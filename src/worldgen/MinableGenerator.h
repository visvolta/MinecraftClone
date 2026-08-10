#pragma once

#include "Block.h"

class JavaRandom;
class WorldGenerationContext;

class MinableGenerator
{
public:
    MinableGenerator(BlockType generatedBlock, int veinSize);

    void generate(
        WorldGenerationContext& context,
        JavaRandom& random,
        int worldX,
        int worldY,
        int worldZ) const;

private:
    BlockType generatedBlock_;
    int veinSize_ = 0;
};
