#include "worldgen/BiomeMap.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
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

std::uint32_t coordinateRandom(
    int seed,
    int x,
    int z,
    std::uint64_t salt) noexcept
{
    std::uint64_t value = static_cast<std::uint64_t>(
        static_cast<std::int64_t>(seed)
    );
    value ^= static_cast<std::uint64_t>(static_cast<std::int64_t>(x)) *
        341873128712ULL;
    value ^= static_cast<std::uint64_t>(static_cast<std::int64_t>(z)) *
        132897987541ULL;
    value ^= salt * 0x9E3779B97F4A7C15ULL;
    value ^= value >> 30U;
    value *= 0xBF58476D1CE4E5B9ULL;
    value ^= value >> 27U;
    value *= 0x94D049BB133111EBULL;
    value ^= value >> 31U;
    return static_cast<std::uint32_t>(value >> 32U);
}

BiomeId selectWeightedBiome(
    BiomeClimateCategory climate,
    std::uint32_t choice) noexcept
{
    if (climate == BiomeClimateCategory::None)
        return VanillaBiomes::Plains;
    const std::size_t climateIndex = static_cast<std::size_t>(climate) - 1U;
    int totalWeight = 0;
    for (const BiomeDefinition& biome : BiomeRegistry::active().entries())
        totalWeight += std::max(0, biome.generationWeights[climateIndex]);
    if (totalWeight <= 0)
        return VanillaBiomes::Plains;

    int selected = static_cast<int>(choice % static_cast<std::uint32_t>(
        totalWeight
    ));
    for (const BiomeDefinition& biome : BiomeRegistry::active().entries())
    {
        selected -= std::max(0, biome.generationWeights[climateIndex]);
        if (selected < 0)
            return biome.id;
    }
    return VanillaBiomes::Plains;
}

BiomeId hillVariant(BiomeId biome) noexcept
{
    switch (biome)
    {
        case VanillaBiomes::Desert: return VanillaBiomes::DesertHills;
        case VanillaBiomes::Forest: return VanillaBiomes::ForestHills;
        case VanillaBiomes::BirchForest:
            return VanillaBiomes::BirchForestHills;
        case VanillaBiomes::Taiga: return VanillaBiomes::TaigaHills;
        case VanillaBiomes::ColdTaiga: return VanillaBiomes::ColdTaigaHills;
        case VanillaBiomes::Jungle: return VanillaBiomes::JungleHills;
        case VanillaBiomes::MegaTaiga:
            return VanillaBiomes::MegaTaigaHills;
        case VanillaBiomes::ExtremeHills:
            return VanillaBiomes::ExtremeHillsPlus;
        case VanillaBiomes::Savanna: return VanillaBiomes::SavannaPlateau;
        case VanillaBiomes::Mesa:
            return VanillaBiomes::MesaPlateauF;
        default: return biome;
    }
}

BiomeId mutationVariant(BiomeId biome) noexcept
{
    const BiomeRegistry& registry = BiomeRegistry::active();
    for (const BiomeDefinition& candidate : registry.entries())
        if (candidate.mutationOf && *candidate.mutationOf == biome)
            return candidate.id;
    return biome;
}
}

BiomeMap::BiomeMap(int seed)
    : seed_(seed),
      temperatureRandom_(
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
        const int gridX = originX + static_cast<int>(index / depth);
        const int gridZ = originZ + static_cast<int>(index % depth);
        const int worldX = gridX * spacing;
        const int worldZ = gridZ * spacing;
        const std::uint32_t primaryChoice = coordinateRandom(
            seed_, worldX >> 2, worldZ >> 2, 200U
        );

        BiomeId biome = VanillaBiomes::Plains;
        if (terrain < -0.66)
        {
            biome = VanillaBiomes::DeepOcean;
        }
        else if (terrain < -0.43)
        {
            biome = temperature < 0.15
                ? VanillaBiomes::FrozenOcean
                : VanillaBiomes::Ocean;
        }
        else if (terrain < -0.33)
        {
            biome = temperature < 0.15
                ? VanillaBiomes::ColdBeach
                : VanillaBiomes::Beach;
        }
        else
        {
            BiomeClimateCategory category = BiomeClimateCategory::Ice;
            if (temperature >= 0.72)
                category = BiomeClimateCategory::Warm;
            else if (temperature >= 0.48)
                category = BiomeClimateCategory::Medium;
            else if (temperature >= 0.25)
                category = BiomeClimateCategory::Cold;

            const bool special = coordinateRandom(
                seed_, worldX >> 4, worldZ >> 4, 3U
            ) % 13U == 0U;
            if (special && category == BiomeClimateCategory::Warm)
            {
                biome = primaryChoice % 3U == 0U
                    ? VanillaBiomes::MesaPlateau
                    : VanillaBiomes::MesaPlateauF;
            }
            else if (special && category == BiomeClimateCategory::Medium)
            {
                biome = VanillaBiomes::Jungle;
            }
            else if (special && category == BiomeClimateCategory::Cold)
            {
                biome = VanillaBiomes::MegaTaiga;
            }
            else
            {
                biome = selectWeightedBiome(category, primaryChoice);
            }

            if (coordinateRandom(
                    seed_, worldX >> 3, worldZ >> 3, 1000U
                ) % 3U == 0U && std::abs(detail[index]) > 0.18)
            {
                biome = hillVariant(biome);
            }
            if (coordinateRandom(
                    seed_, worldX >> 3, worldZ >> 3, 1001U
                ) % 29U == 1U)
            {
                biome = mutationVariant(biome);
            }

            // GenLayerRiverMix replaces non-ocean land after hills and shore.
            if (std::abs(detail[index]) < 0.018 &&
                biome != VanillaBiomes::MushroomIsland &&
                biome != VanillaBiomes::MushroomShore)
            {
                biome = temperature < 0.15
                    ? VanillaBiomes::FrozenRiver
                    : VanillaBiomes::River;
            }
        }
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
