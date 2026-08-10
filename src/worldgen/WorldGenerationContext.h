#pragma once

#include "Block.h"
#include "worldgen/Biome.h"

#include <cstddef>
#include <functional>
#include <unordered_map>

class Chunk;

class WorldGenerationContext
{
public:
    using FallbackSampler = std::function<BlockType(int, int, int)>;
    using HeightSampler = std::function<int(int, int)>;
    using ClimateSampler = std::function<ClimateSample(int, int)>;

    WorldGenerationContext(
        Chunk& targetChunk,
        FallbackSampler fallbackSampler,
        HeightSampler heightSampler,
        ClimateSampler climateSampler
    );

    [[nodiscard]] BlockType getBlock(int worldX, int worldY, int worldZ) const;
    [[nodiscard]] bool isInsideTarget(int worldX, int worldY, int worldZ) const noexcept;

    bool setBlock(int worldX, int worldY, int worldZ, BlockType block);
    void beginIsolatedFeature();
    void finishIsolatedFeature(bool commit);
    [[nodiscard]] int getHeightValue(int worldX, int worldZ) const;
    [[nodiscard]] ClimateSample sampleClimate(int worldX, int worldZ) const;

private:
    struct FeaturePosition
    {
        int x = 0;
        int y = 0;
        int z = 0;

        bool operator==(const FeaturePosition&) const noexcept = default;
    };

    struct FeaturePositionHash
    {
        [[nodiscard]] std::size_t operator()(
            const FeaturePosition& position) const noexcept;
    };

    Chunk& targetChunk_;
    FallbackSampler fallbackSampler_;
    HeightSampler heightSampler_;
    ClimateSampler climateSampler_;
    std::unordered_map<FeaturePosition, BlockType, FeaturePositionHash>
        stagedFeatureBlocks_;
    bool isolatedFeatureActive_ = false;
};
