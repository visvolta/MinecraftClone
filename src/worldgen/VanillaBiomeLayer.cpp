#include "worldgen/VanillaBiomeLayer.h"

#include <algorithm>
#include <bit>
#include <cassert>
#include <cstddef>

namespace
{
constexpr std::uint64_t Multiplier = 6364136223846793005ULL;
constexpr std::uint64_t Addend = 1442695040888963407ULL;

int floorDivide(int value, int divisor) noexcept
{
    int quotient = value / divisor;
    if (value % divisor < 0)
        --quotient;
    return quotient;
}

int positiveModulo(int value, int divisor) noexcept
{
    const int remainder = value % divisor;
    return remainder < 0 ? remainder + divisor : remainder;
}
}

std::uint64_t VanillaLayerRandom::mix(
    std::uint64_t value,
    std::uint64_t addend) noexcept
{
    return value * (value * Multiplier + Addend) + addend;
}

VanillaLayerRandom::VanillaLayerRandom(
    std::int64_t layerSeed,
    std::int64_t worldSeed) noexcept
{
    const std::uint64_t seed = static_cast<std::uint64_t>(layerSeed);
    baseSeed_ = seed;
    for (int round = 0; round < 3; ++round)
        baseSeed_ = mix(baseSeed_, seed);

    worldSeed_ = static_cast<std::uint64_t>(worldSeed);
    for (int round = 0; round < 3; ++round)
        worldSeed_ = mix(worldSeed_, baseSeed_);
}

void VanillaLayerRandom::seedCell(std::int64_t x, std::int64_t z) noexcept
{
    cellSeed_ = worldSeed_;
    cellSeed_ = mix(cellSeed_, static_cast<std::uint64_t>(x));
    cellSeed_ = mix(cellSeed_, static_cast<std::uint64_t>(z));
    cellSeed_ = mix(cellSeed_, static_cast<std::uint64_t>(x));
    cellSeed_ = mix(cellSeed_, static_cast<std::uint64_t>(z));
}

int VanillaLayerRandom::nextInt(int bound) noexcept
{
    assert(bound > 0);
    const std::int64_t signedSeed = std::bit_cast<std::int64_t>(cellSeed_);
    int value = static_cast<int>((signedSeed >> 24) % bound);
    if (value < 0)
        value += bound;
    cellSeed_ = mix(cellSeed_, worldSeed_);
    return value;
}

std::vector<BiomeId> vanillaVoronoiZoom(
    std::int64_t worldSeed,
    int originX,
    int originZ,
    int width,
    int depth,
    const GenerationBiomeSampler& parentSampler)
{
    if (width <= 0 || depth <= 0)
        return {};

    const int adjustedX = originX - 2;
    const int adjustedZ = originZ - 2;
    const int parentX = floorDivide(adjustedX, 4);
    const int parentZ = floorDivide(adjustedZ, 4);
    const int parentWidth = width / 4 + 2;
    const int parentDepth = depth / 4 + 2;
    const std::vector<BiomeId> parent = parentSampler(
        parentX, parentZ, parentWidth, parentDepth
    );
    if (parent.size() != static_cast<std::size_t>(parentWidth * parentDepth))
        return std::vector<BiomeId>(
            static_cast<std::size_t>(width * depth), VanillaBiomes::Plains
        );

    const int expandedWidth = (parentWidth - 1) * 4;
    const int expandedDepth = (parentDepth - 1) * 4;
    std::vector<BiomeId> expanded(
        static_cast<std::size_t>(expandedWidth * expandedDepth)
    );
    const auto parentAt = [&](int x, int z)
    {
        return parent[static_cast<std::size_t>(x * parentDepth + z)] & 255U;
    };
    const auto writeExpanded = [&](int x, int z, BiomeId biome)
    {
        expanded[static_cast<std::size_t>(z * expandedWidth + x)] = biome;
    };

    VanillaLayerRandom random(10, worldSeed);
    for (int cellZ = 0; cellZ < parentDepth - 1; ++cellZ)
    {
        for (int cellX = 0; cellX < parentWidth - 1; ++cellX)
        {
            const BiomeId northWest = parentAt(cellX, cellZ);
            const BiomeId northEast = parentAt(cellX + 1, cellZ);
            const BiomeId southWest = parentAt(cellX, cellZ + 1);
            const BiomeId southEast = parentAt(cellX + 1, cellZ + 1);

            random.seedCell(
                static_cast<std::int64_t>(cellX + parentX) * 4,
                static_cast<std::int64_t>(cellZ + parentZ) * 4
            );
            const double nwX = (random.nextInt(1024) / 1024.0 - 0.5) * 3.6;
            const double nwZ = (random.nextInt(1024) / 1024.0 - 0.5) * 3.6;
            random.seedCell(
                static_cast<std::int64_t>(cellX + parentX + 1) * 4,
                static_cast<std::int64_t>(cellZ + parentZ) * 4
            );
            const double neX =
                (random.nextInt(1024) / 1024.0 - 0.5) * 3.6 + 4.0;
            const double neZ = (random.nextInt(1024) / 1024.0 - 0.5) * 3.6;
            random.seedCell(
                static_cast<std::int64_t>(cellX + parentX) * 4,
                static_cast<std::int64_t>(cellZ + parentZ + 1) * 4
            );
            const double swX = (random.nextInt(1024) / 1024.0 - 0.5) * 3.6;
            const double swZ =
                (random.nextInt(1024) / 1024.0 - 0.5) * 3.6 + 4.0;
            random.seedCell(
                static_cast<std::int64_t>(cellX + parentX + 1) * 4,
                static_cast<std::int64_t>(cellZ + parentZ + 1) * 4
            );
            const double seX =
                (random.nextInt(1024) / 1024.0 - 0.5) * 3.6 + 4.0;
            const double seZ =
                (random.nextInt(1024) / 1024.0 - 0.5) * 3.6 + 4.0;

            for (int localZ = 0; localZ < 4; ++localZ)
            {
                for (int localX = 0; localX < 4; ++localX)
                {
                    const auto distanceSquared = [=](double x, double z)
                    {
                        const double dx = localX - x;
                        const double dz = localZ - z;
                        return dx * dx + dz * dz;
                    };
                    const double nw = distanceSquared(nwX, nwZ);
                    const double ne = distanceSquared(neX, neZ);
                    const double sw = distanceSquared(swX, swZ);
                    const double se = distanceSquared(seX, seZ);
                    const BiomeId selected = nw < ne && nw < sw && nw < se
                        ? northWest
                        : ne < nw && ne < sw && ne < se
                            ? northEast
                            : sw < nw && sw < ne && sw < se
                                ? southWest : southEast;
                    writeExpanded(
                        cellX * 4 + localX,
                        cellZ * 4 + localZ,
                        selected
                    );
                }
            }
        }
    }

    const int offsetX = positiveModulo(adjustedX, 4);
    const int offsetZ = positiveModulo(adjustedZ, 4);
    std::vector<BiomeId> result(static_cast<std::size_t>(width * depth));
    for (int x = 0; x < width; ++x)
    for (int z = 0; z < depth; ++z)
    {
        const int sourceX = x + offsetX;
        const int sourceZ = z + offsetZ;
        result[static_cast<std::size_t>(x * depth + z)] =
            expanded[static_cast<std::size_t>(
                sourceZ * expandedWidth + sourceX
            )];
    }
    return result;
}
