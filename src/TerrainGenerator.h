#pragma once

#include "Block.h"
#include "worldgen/BetaNoiseGeneratorOctaves.h"
#include "worldgen/BetaSimplexOctaves.h"
#include "worldgen/BiomeMap.h"
#include "worldgen/CaveGenerator.h"
#include "worldgen/JavaRandom.h"
#include "worldgen/PopulationGenerator.h"
#include "worldgen/RavineGenerator.h"
#include "worldgen/StructureGenerator.h"
#include "worldgen/SurfaceBuilder.h"

#include <cstdint>
#include <deque>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

class Chunk;

class TerrainGenerator
{
public:
    static constexpr std::uint32_t CURRENT_GENERATION_VERSION = 5;
    explicit TerrainGenerator(int seed = 1337);
    ~TerrainGenerator();

    void generateChunk(Chunk& chunk) const;

    [[nodiscard]] int getSeed() const noexcept;
    [[nodiscard]] int getTerrainHeight(int worldX, int worldZ) const;
    [[nodiscard]] std::optional<StructureLocation> findNearestStructure(
        WorldStructure structure,
        int worldX,
        int worldZ,
        int maximumRegionRadius = 100) const;

private:
    int seed_ = 1337;

    // ChunkGeneratorOverworld constructs every noise generator from one
    // java.util.Random in this exact order. Changing this order changes the
    // entire world for a seed.
    mutable JavaRandom generatorRandom_;
    mutable JavaRandom chunkRandom_;

    BetaNoiseGeneratorOctaves minLimitNoise_;
    BetaNoiseGeneratorOctaves maxLimitNoise_;
    BetaNoiseGeneratorOctaves mainNoise_;
    BetaSimplexOctaves surfaceDepthNoise_;
    BetaNoiseGeneratorOctaves scaleNoise_;
    BetaNoiseGeneratorOctaves depthNoise_;
    BetaNoiseGeneratorOctaves mobSpawnerNoise_;

    BiomeMap biomeMap_;
    SurfaceBuilder surfaceBuilder_;
    CaveGenerator caveGenerator_;
    RavineGenerator ravineGenerator_;
    StructureGenerator structureGenerator_;
    PopulationGenerator populationGenerator_;

    mutable std::vector<double> densityField_;
    mutable std::vector<double> mainField_;
    mutable std::vector<double> minLimitField_;
    mutable std::vector<double> maxLimitField_;
    mutable std::vector<double> scaleField_;
    mutable std::vector<double> depthField_;
    mutable std::vector<double> surfaceDepthField_;

    mutable std::unordered_map<std::uint64_t, std::unique_ptr<Chunk>> terrainCache_;
    mutable std::deque<std::uint64_t> terrainCacheOrder_;

    void generateTerrainOnly(Chunk& chunk) const;
    [[nodiscard]] const Chunk& terrainChunkAt(int chunkX, int chunkZ) const;
    void cacheTerrainChunk(const Chunk& chunk) const;
    [[nodiscard]] static std::uint64_t terrainCacheKey(int chunkX, int chunkZ) noexcept;
    [[nodiscard]] static int floorDivide(int value, int divisor) noexcept;
    [[nodiscard]] static int positiveModulo(int value, int divisor) noexcept;

    void generateBaseTerrain(
        Chunk& chunk,
        const std::vector<ClimateSample>& climate) const;
    void replaceSurfaceBlocks(
        Chunk& chunk,
        const std::vector<ClimateSample>& climate) const;
    void initializeNoiseField(
        int originX,
        int originY,
        int originZ,
        int sizeX,
        int sizeY,
        int sizeZ,
        const std::vector<ClimateSample>& climate) const;

    [[nodiscard]] static long long makeChunkSeed(int chunkX, int chunkZ) noexcept;
};
