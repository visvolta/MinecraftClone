#include "worldgen/BiomeMap.h"

#include <algorithm>
#include <bit>
#include <cstdint>

namespace
{
std::int64_t multiplySeed(
    int seed,
    std::int64_t multiplier) noexcept
{
    const std::uint64_t value =
        static_cast<std::uint64_t>(
            static_cast<std::int64_t>(seed)
        ) *
        static_cast<std::uint64_t>(multiplier);

    return std::bit_cast<std::int64_t>(value);
}
}

BiomeMap::BiomeMap(int seed)
    : temperatureRandom_(
          multiplySeed(seed, 9871LL)),
      humidityRandom_(
          multiplySeed(seed, 39811LL)),
      detailRandom_(
          multiplySeed(seed, 543321LL)),
      continentalRandom_(
          multiplySeed(seed, 918273645LL)),
      temperatureNoise_(temperatureRandom_, 4),
      humidityNoise_(humidityRandom_, 4),
      detailNoise_(detailRandom_, 2),
      continentalNoise_(continentalRandom_, 4)
{
}

ClimateSample BiomeMap::sample(
    int worldX,
    int worldZ) const
{
    return sampleArea(
        worldX,
        worldZ,
        1,
        1
    ).front();
}

std::vector<ClimateSample> BiomeMap::sampleArea(
    int originX,
    int originZ,
    int width,
    int depth) const
{
    return sampleGrid(originX, originZ, width, depth, 1);
}

std::vector<ClimateSample> BiomeMap::sampleGenerationArea(
    int originCellX,
    int originCellZ,
    int width,
    int depth) const
{
    return sampleGrid(originCellX, originCellZ, width, depth, 4);
}

std::vector<ClimateSample> BiomeMap::sampleGrid(
    int originX,
    int originZ,
    int width,
    int depth,
    int spacing) const
{
    std::vector<double> temperatures;
    std::vector<double> humidities;
    std::vector<double> detail;
    std::vector<double> continentalness;

    const double sampleSpacing = static_cast<double>(spacing);

    temperatureNoise_.generate(
        temperatures,
        static_cast<double>(originX),
        static_cast<double>(originZ),
        width,
        depth,
        0.02500000037252903 * sampleSpacing,
        0.02500000037252903 * sampleSpacing,
        0.25
    );

    humidityNoise_.generate(
        humidities,
        static_cast<double>(originX),
        static_cast<double>(originZ),
        width,
        depth,
        0.05000000074505806 * sampleSpacing,
        0.05000000074505806 * sampleSpacing,
        0.3333333333333333
    );

    detailNoise_.generate(
        detail,
        static_cast<double>(originX),
        static_cast<double>(originZ),
        width,
        depth,
        0.25 * sampleSpacing,
        0.25 * sampleSpacing,
        0.5882352941176471
    );

    continentalNoise_.generate(
        continentalness,
        static_cast<double>(originX),
        static_cast<double>(originZ),
        width,
        depth,
        (1.0 / 512.0) * sampleSpacing,
        (1.0 / 512.0) * sampleSpacing,
        0.5,
        0.5
    );

    std::vector<ClimateSample> samples;
    samples.resize(
        static_cast<std::size_t>(width * depth)
    );

    for (std::size_t index = 0;
         index < samples.size();
         ++index)
    {
        const double detailValue =
            detail[index] * 1.1 + 0.5;

        constexpr double temperatureDetailWeight = 0.01;
        constexpr double humidityDetailWeight = 0.002;

        double temperature =
            (temperatures[index] * 0.15 + 0.7) *
                (1.0 - temperatureDetailWeight) +
            detailValue * temperatureDetailWeight;

        double humidity =
            (humidities[index] * 0.15 + 0.5) *
                (1.0 - humidityDetailWeight) +
            detailValue * humidityDetailWeight;

        temperature =
            1.0 -
            (1.0 - temperature) *
                (1.0 - temperature);

        temperature =
            std::clamp(temperature, 0.0, 1.0);
        humidity =
            std::clamp(humidity, 0.0, 1.0);

        const double terrain = std::clamp(
            continentalness[index] * 0.85 + detail[index] * 0.15,
            -1.0,
            1.0
        );

        const BiomeId biome = classifyReleaseBiome(
            temperature,
            humidity,
            terrain
        );
        const BiomeDefinition* definition =
            BiomeRegistry::active().find(biome);

        samples[index] = {
            definition == nullptr ? temperature : definition->temperature,
            definition == nullptr ? humidity : definition->rainfall,
            biome
        };
    }

    return samples;
}
