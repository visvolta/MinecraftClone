#pragma once

#include "Block.h"
#include "worldgen/Biome.h"

#include <functional>

class Chunk;

class WorldGenerationContext
{
public:
    using FallbackSampler = std::function<BlockType(int, int, int)>;

    WorldGenerationContext(Chunk& targetChunk, FallbackSampler fallbackSampler);

    [[nodiscard]] BlockType getBlock(int worldX, int worldY, int worldZ) const;
    [[nodiscard]] bool isInsideTarget(int worldX, int worldY, int worldZ) const noexcept;

    bool setBlock(int worldX, int worldY, int worldZ, BlockType block);
    [[nodiscard]] int getHeightValue(int worldX, int worldZ) const;
    [[nodiscard]] ClimateSample sampleClimate(int worldX, int worldZ) const;

private:
    Chunk& targetChunk_;
    FallbackSampler fallbackSampler_;
};
