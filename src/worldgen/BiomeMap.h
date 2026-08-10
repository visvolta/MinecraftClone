#pragma once

#include "worldgen/BetaSimplexOctaves.h"
#include "worldgen/Biome.h"
#include "worldgen/JavaRandom.h"

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

private:
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
