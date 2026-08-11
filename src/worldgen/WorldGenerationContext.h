#pragma once

#include "Block.h"
#include "content/BlockState.h"
#include "worldgen/Biome.h"

#include <cstddef>
#include <functional>
#include <cstdint>
#include <string_view>
#include <unordered_map>
#include <utility>

class Chunk;

class WorldGenerationContext
{
public:
    // Compatibility overload for old generators that still expose only a
    // legacy BlockType fallback. New 1.12.2 code must use StateFallbackSampler
    // so off-chunk reads retain variant/property data.
    using FallbackSampler = std::function<BlockType(int, int, int)>;
    using StateFallbackSampler =
        std::function<mc::content::BlockState(int, int, int)>;
    using HeightSampler = std::function<int(int, int)>;
    using ClimateSampler = std::function<ClimateSample(int, int)>;
    using StructureMobHook = std::function<void(std::string_view,int,int,int)>;
    using StructureLootHook = std::function<void(int,int,int,std::string_view,std::int64_t)>;
    using StructureSpawnerHook = std::function<void(int,int,int,std::string_view)>;

    WorldGenerationContext(
        Chunk& targetChunk,
        FallbackSampler fallbackSampler,
        HeightSampler heightSampler,
        ClimateSampler climateSampler
    );
    WorldGenerationContext(
        Chunk& targetChunk,
        StateFallbackSampler fallbackSampler,
        HeightSampler heightSampler,
        ClimateSampler climateSampler
    );

    [[nodiscard]] BlockType getBlock(int worldX, int worldY, int worldZ) const;
    [[nodiscard]] mc::content::BlockState getBlockState(
        int worldX, int worldY, int worldZ
    ) const;
    [[nodiscard]] bool isInsideTarget(
        int worldX, int worldY, int worldZ
    ) const noexcept;

    bool setBlock(int worldX, int worldY, int worldZ, BlockType block);
    bool setBlockState(
        int worldX,
        int worldY,
        int worldZ,
        mc::content::BlockState state
    );
    void beginIsolatedFeature();
    void finishIsolatedFeature(bool commit);
    [[nodiscard]] int getHeightValue(int worldX, int worldZ) const;
    [[nodiscard]] int getTopSolidOrLiquidBlockY(int worldX, int worldZ) const;
    [[nodiscard]] ClimateSample sampleClimate(int worldX, int worldZ) const;

    void setStructureMobHook(StructureMobHook hook) { structureMobHook_ = std::move(hook); }
    void setStructureLootHook(StructureLootHook hook) { structureLootHook_ = std::move(hook); }
    void setStructureSpawnerHook(StructureSpawnerHook hook) { structureSpawnerHook_ = std::move(hook); }
    void spawnStructureMob(std::string_view id,int x,int y,int z) const;
    void assignStructureLoot(int x,int y,int z,std::string_view table,std::int64_t seed) const;
    void assignStructureSpawner(int x,int y,int z,std::string_view entityId) const;

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
            const FeaturePosition& position
        ) const noexcept;
    };

    Chunk& targetChunk_;
    StateFallbackSampler fallbackSampler_;
    HeightSampler heightSampler_;
    ClimateSampler climateSampler_;
    std::unordered_map<
        FeaturePosition,
        mc::content::BlockState,
        FeaturePositionHash
    > stagedFeatureBlocks_;
    bool isolatedFeatureActive_ = false;
    StructureMobHook structureMobHook_;
    StructureLootHook structureLootHook_;
    StructureSpawnerHook structureSpawnerHook_;
};
