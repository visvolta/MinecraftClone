#pragma once

#include "worldgen/Biome.h"

#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

class BiomeMap
{
public:
    explicit BiomeMap(std::int64_t seed);

    [[nodiscard]] ClimateSample sample(int worldX, int worldZ) const;

    [[nodiscard]] std::vector<ClimateSample> sampleArea(
        int originX,
        int originZ,
        int width,
        int depth) const;

    [[nodiscard]] std::vector<ClimateSample> sampleGenerationArea(
        int originCellX,
        int originCellZ,
        int width,
        int depth) const;

    [[nodiscard]] std::optional<std::pair<int, int>> findSpawnBiomePosition(
        int range = 256) const;

private:
    std::int64_t seed_ = 0;

    [[nodiscard]] static ClimateSample climateFor(BiomeId biome) noexcept;
    [[nodiscard]] static int floorDivide(int value, int divisor) noexcept;
};
