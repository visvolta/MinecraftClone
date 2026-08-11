#include "worldgen/BiomeMap.h"

#include "worldgen/JavaRandom.h"
#include "worldgen/VanillaBiomeLayer.h"

#include <algorithm>
#include <cstddef>

namespace
{
bool isSpawnBiome(BiomeId biome) noexcept
{
    switch (biome)
    {
        case VanillaBiomes::Forest:
        case VanillaBiomes::Plains:
        case VanillaBiomes::Taiga:
        case VanillaBiomes::TaigaHills:
        case VanillaBiomes::ForestHills:
        case VanillaBiomes::Jungle:
        case VanillaBiomes::JungleHills:
            return true;
        default:
            return false;
    }
}
}

BiomeMap::BiomeMap(std::int64_t seed) : seed_(seed) {}

ClimateSample BiomeMap::climateFor(BiomeId biome) noexcept
{
    const BiomeDefinition* definition = BiomeRegistry::active().find(biome);
    return {
        definition == nullptr ? 0.5 : static_cast<double>(definition->temperature),
        definition == nullptr ? 0.5 : static_cast<double>(definition->rainfall),
        biome
    };
}

ClimateSample BiomeMap::sample(int worldX, int worldZ) const
{
    const std::vector<BiomeId> biomes = vanillaVoronoiBiomes(
        seed_, worldX, worldZ, 1, 1);
    return climateFor(biomes.empty() ? VanillaBiomes::Plains : biomes.front());
}

std::vector<ClimateSample> BiomeMap::sampleArea(
    int originX,
    int originZ,
    int width,
    int depth) const
{
    const std::vector<BiomeId> biomes = vanillaVoronoiBiomes(
        seed_, originX, originZ, width, depth);
    std::vector<ClimateSample> result;
    result.reserve(biomes.size());
    for (const BiomeId biome : biomes)
        result.push_back(climateFor(biome));
    return result;
}

std::vector<ClimateSample> BiomeMap::sampleGenerationArea(
    int originCellX,
    int originCellZ,
    int width,
    int depth) const
{
    const std::vector<BiomeId> biomes = vanillaGenerationBiomes(
        seed_, originCellX, originCellZ, width, depth);
    std::vector<ClimateSample> result;
    result.reserve(biomes.size());
    for (const BiomeId biome : biomes)
        result.push_back(climateFor(biome));
    return result;
}

std::optional<std::pair<int, int>> BiomeMap::findSpawnBiomePosition(int range) const
{
    range = std::max(0, range);
    const int minCellX = floorDivide(-range, 4);
    const int minCellZ = floorDivide(-range, 4);
    const int maxCellX = floorDivide(range, 4);
    const int maxCellZ = floorDivide(range, 4);
    const int width = maxCellX - minCellX + 1;
    const int depth = maxCellZ - minCellZ + 1;
    const std::vector<BiomeId> biomes = vanillaGenerationBiomes(
        seed_, minCellX, minCellZ, width, depth);

    JavaRandom random(seed_);
    std::optional<std::pair<int, int>> selected;
    int matches = 0;

    // BiomeProvider::findBiomePosition walks the GenLayer result in Java's
    // row-major order (X changes fastest), so preserve that RNG sequence even
    // though the project's public biome arrays are x-major.
    for (int z = 0; z < depth; ++z)
    {
        for (int x = 0; x < width; ++x)
        {
            const BiomeId biome = biomes[static_cast<std::size_t>(x * depth + z)];
            if (!isSpawnBiome(biome))
                continue;
            if (!selected || random.nextInt(matches + 1) == 0)
                selected = std::pair{(minCellX + x) << 2, (minCellZ + z) << 2};
            ++matches;
        }
    }
    return selected;
}

int BiomeMap::floorDivide(int value, int divisor) noexcept
{
    int quotient = value / divisor;
    const int remainder = value % divisor;
    if (remainder != 0 && ((remainder < 0) != (divisor < 0)))
        --quotient;
    return quotient;
}
