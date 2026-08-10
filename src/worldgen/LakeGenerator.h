#pragma once

#include "Block.h"

class JavaRandom;
class WorldGenerationContext;

class LakeGenerator
{
public:
    explicit LakeGenerator(BlockType liquid = BlockType::Water);

    bool generate(
        WorldGenerationContext& context,
        JavaRandom& random,
        int worldX,
        int worldY,
        int worldZ) const;

private:
    BlockType liquid_;
};
