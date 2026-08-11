#pragma once

#include "worldgen/Biome.h"

#include <cstdint>
#include <functional>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

class Chunk;
class JavaRandom;
class WorldGenerationContext;

enum class WorldStructure : std::uint8_t
{
    Mineshaft,
    Village,
    Temple,
    Stronghold,
    OceanMonument,
    WoodlandMansion
};

struct StructureLocation
{
    WorldStructure type = WorldStructure::Village;
    int blockX = 0;
    int blockZ = 0;
    BiomeId biome = VanillaBiomes::Plains;
};

[[nodiscard]] const char* structureName(WorldStructure structure) noexcept;
[[nodiscard]] std::optional<WorldStructure> parseStructureName(
    std::string_view name) noexcept;

class StructureGenerator
{
public:
    using ClimateSampler = std::function<ClimateSample(int, int)>;

    explicit StructureGenerator(std::int64_t worldSeed);
    void populate(
        Chunk& targetChunk,
        WorldGenerationContext& context) const;
    [[nodiscard]] std::optional<StructureLocation> findNearest(
        WorldStructure structure,
        int blockX,
        int blockZ,
        int maximumRegionRadius,
        const ClimateSampler& climateSampler) const;

private:
    std::int64_t worldSeed_ = 0;
    std::vector<std::pair<int, int>> strongholdChunks_;

    [[nodiscard]] bool isVillageChunk(int chunkX, int chunkZ) const;
    [[nodiscard]] bool isTempleChunk(int chunkX, int chunkZ) const;
    [[nodiscard]] bool isMineshaftChunk(int chunkX, int chunkZ) const;
    [[nodiscard]] bool isStrongholdChunk(int chunkX, int chunkZ) const;
    [[nodiscard]] bool isOceanMonumentChunk(int chunkX, int chunkZ) const;
    [[nodiscard]] bool isWoodlandMansionChunk(int chunkX, int chunkZ) const;
    void generateMineshaft(
        WorldGenerationContext&, JavaRandom&, int, int) const;
    void generateVillage(
        WorldGenerationContext&, JavaRandom&, int, int, BiomeId) const;
    void generateTemple(
        WorldGenerationContext&, JavaRandom&, int, int, BiomeId) const;
    void generateStronghold(
        WorldGenerationContext&, JavaRandom&, int, int) const;
    void generateOceanMonument(
        WorldGenerationContext&, JavaRandom&, int, int) const;
    void generateWoodlandMansion(
        WorldGenerationContext&, JavaRandom&, int, int) const;
};
