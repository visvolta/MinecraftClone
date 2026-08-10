#pragma once

#include "Block.h"
#include "worldgen/BetaNoiseGeneratorOctaves.h"
#include "worldgen/BiomeMap.h"
#include "worldgen/CaveGenerator.h"
#include "worldgen/JavaRandom.h"
#include "worldgen/PopulationGenerator.h"
#include "worldgen/SurfaceBuilder.h"

#include <vector>

class Chunk;

class TerrainGenerator
{
public:
    explicit TerrainGenerator(int seed = 1337);

    void generateChunk(Chunk& chunk) const;

    [[nodiscard]] int getSeed() const noexcept;
    [[nodiscard]] int getTerrainHeight(
        int worldX,
        int worldZ) const;

private:
    int seed_ = 1337;

    // The octave generators consume one shared JavaRandom in the exact order
    // used by ChunkProviderGenerate's constructor.
    mutable JavaRandom generatorRandom_;
    mutable JavaRandom chunkRandom_;

    BetaNoiseGeneratorOctaves minLimitNoise_;
    BetaNoiseGeneratorOctaves maxLimitNoise_;
    BetaNoiseGeneratorOctaves mainNoise_;
    BetaNoiseGeneratorOctaves surfaceDepthNoise_;
    BetaNoiseGeneratorOctaves scaleNoise_;
    BetaNoiseGeneratorOctaves depthNoise_;
    BetaNoiseGeneratorOctaves mobSpawnerNoise_;

    BiomeMap biomeMap_;
    SurfaceBuilder surfaceBuilder_;
    CaveGenerator caveGenerator_;
    PopulationGenerator populationGenerator_;

    mutable std::vector<double> densityField_;
    mutable std::vector<double> mainField_;
    mutable std::vector<double> minLimitField_;
    mutable std::vector<double> maxLimitField_;
    mutable std::vector<double> scaleField_;
    mutable std::vector<double> depthField_;
    mutable std::vector<double> surfaceDepthField_;

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

    [[nodiscard]] BlockType sampleBaseBlock(
        int worldX,
        int worldY,
        int worldZ) const;

    [[nodiscard]] static long long makeChunkSeed(
        int chunkX,
        int chunkZ) noexcept;
};
