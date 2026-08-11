#pragma once

#include "Block.h"
#include "worldgen/Biome.h"

#include <cstdint>

class Chunk;
class JavaRandom;

class SurfaceBuilder
{
public:
    static constexpr int SEA_LEVEL = 63;
    explicit SurfaceBuilder(std::int64_t worldSeed = 0) : worldSeed_(worldSeed) {}

    void replaceColumn(Chunk& chunk,int localX,int localZ,
        const ClimateSample& climate,double sandNoise,double gravelNoise,
        double stoneNoise,JavaRandom& random) const;

    [[nodiscard]] static BlockType biomeTopBlock(BiomeId biome) noexcept;
    [[nodiscard]] static BlockType biomeFillerBlock(BiomeId biome) noexcept;
private:
    std::int64_t worldSeed_ = 0;
};
