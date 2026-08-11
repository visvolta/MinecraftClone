#pragma once

#include "worldgen/Biome.h"

#include <cstdint>
#include <vector>

// Minecraft 1.12.2 GenLayer implementation for the default Overworld.
// Public results use this project's x-major layout: x * depth + z.
[[nodiscard]] std::vector<BiomeId> vanillaGenerationBiomes(
    std::int64_t worldSeed,
    int originX,
    int originZ,
    int width,
    int depth);

[[nodiscard]] std::vector<BiomeId> vanillaVoronoiBiomes(
    std::int64_t worldSeed,
    int originX,
    int originZ,
    int width,
    int depth);
