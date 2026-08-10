#pragma once

#include "worldgen/BetaSimplexOctaves.h"
#include "worldgen/Biome.h"
#include "worldgen/JavaRandom.h"

#include <optional>
#include <utility>
#include <vector>

class BiomeMap
{
public:
    explicit BiomeMap(int seed);

    [[nodiscard]] ClimateSample sample(
        int worldX,
        int worldZ) const;

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

    // Mirrors BiomeProvider::findBiomePosition for the Overworld spawn list:
    // choose one suitable generation cell within 256 blocks using reservoir
    // sampling seeded by the world seed.
    [[nodiscard]] std::optional<std::pair<int, int>> findSpawnBiomePosition(
        int range = 256) const;

private:
    int seed_ = 0;
    JavaRandom temperatureRandom_;
    JavaRandom humidityRandom_;
    JavaRandom detailRandom_;
    JavaRandom continentalRandom_;

    BetaSimplexOctaves temperatureNoise_;
    BetaSimplexOctaves humidityNoise_;
    BetaSimplexOctaves detailNoise_;
    BetaSimplexOctaves continentalNoise_;

    [[nodiscard]] std::vector<ClimateSample> sampleGrid(
        int originX,
        int originZ,
        int width,
        int depth,
        int spacing) const;
};
