#pragma once

#include "worldgen/Biome.h"

#include <cstdint>
#include <functional>
#include <vector>

// The 1.12 biome pipeline does not use java.util.Random. GenLayer has its own
// overflowing 64-bit LCG, seeded once per layer and again for every cell.
// Keeping it separate prevents subtle differences in negative coordinates and
// overflow from leaking into the rest of world generation.
class VanillaLayerRandom
{
public:
    VanillaLayerRandom(std::int64_t layerSeed, std::int64_t worldSeed) noexcept;

    void seedCell(std::int64_t x, std::int64_t z) noexcept;
    [[nodiscard]] int nextInt(int bound) noexcept;

private:
    std::uint64_t baseSeed_ = 0;
    std::uint64_t worldSeed_ = 0;
    std::uint64_t cellSeed_ = 0;

    [[nodiscard]] static std::uint64_t mix(
        std::uint64_t value,
        std::uint64_t addend) noexcept;
};

using GenerationBiomeSampler = std::function<std::vector<BiomeId>(
    int originCellX,
    int originCellZ,
    int width,
    int depth)>;

// Exact GenLayerVoronoiZoom coordinate transform and random jitter from 1.12.
// Results use the project's x-major layout: x * depth + z.
[[nodiscard]] std::vector<BiomeId> vanillaVoronoiZoom(
    std::int64_t worldSeed,
    int originX,
    int originZ,
    int width,
    int depth,
    const GenerationBiomeSampler& parentSampler);
