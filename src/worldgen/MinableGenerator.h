#pragma once

#include "Block.h"
#include "content/BlockState.h"

class JavaRandom;
class WorldGenerationContext;

class MinableGenerator
{
public:
    MinableGenerator(BlockType generatedBlock, int veinSize);
    MinableGenerator(mc::content::BlockState generatedState, int veinSize);

    void generate(
        WorldGenerationContext& context,
        JavaRandom& random,
        int worldX,
        int worldY,
        int worldZ) const;

private:
    mc::content::BlockState generatedState_;
    int veinSize_ = 0;
};
