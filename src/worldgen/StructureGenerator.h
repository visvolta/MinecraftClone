#pragma once

#include "worldgen/Biome.h"
#include "worldgen/MineshaftStructure.h"
#include "worldgen/VillageStructure.h"
#include "worldgen/StrongholdStructure.h"
#include "worldgen/ScatteredFeatureStructure.h"
#include "worldgen/OceanMonumentStructure.h"
#include "worldgen/WoodlandMansionStructure.h"

#include <cstdint>
#include <functional>
#include <optional>
#include <memory>
#include <unordered_map>
#include <string_view>
#include <utility>
#include <vector>

class Chunk;
class JavaRandom;
class WorldGenerationContext;


enum class WorldStructure : std::uint8_t
{
    Mineshaft, Village, Temple, Stronghold, OceanMonument, WoodlandMansion
};

struct StructureLocation
{
    WorldStructure type = WorldStructure::Village;
    int blockX = 0;
    int blockZ = 0;
    BiomeId biome = VanillaBiomes::Plains;
};

struct PopulationStructureResult
{
    bool villageGenerated = false;
};

[[nodiscard]] const char* structureName(WorldStructure structure) noexcept;
[[nodiscard]] std::optional<WorldStructure> parseStructureName(std::string_view name) noexcept;

class StructureGenerator
{
public:
    using ClimateSampler = std::function<ClimateSample(int, int)>;
    explicit StructureGenerator(std::int64_t worldSeed);

    // 1.12.2 MapGenStructure post-processing is part of
    // ChunkGeneratorOverworld::populate and consumes the same Random as lakes,
    // dungeons and biome decoration. `sourceChunkX/Z` identify that population
    // pass; the vanilla clipping box is source*16+8 .. +23.
    [[nodiscard]] PopulationStructureResult populateSource(
        WorldGenerationContext& context,
        JavaRandom& populationRandom,
        int sourceChunkX,
        int sourceChunkZ) const;

    [[nodiscard]] std::optional<StructureLocation> findNearest(
        WorldStructure structure, int blockX, int blockZ,
        int maximumRegionRadius, const ClimateSampler& climateSampler) const;

private:
    std::int64_t worldSeed_ = 0;
    std::vector<std::pair<int, int>> strongholdChunks_;

    // MapGenStructure owns persistent StructureStart instances. Keep the same
    // start object across neighbouring population passes so component-local
    // state (ground height, generated chests/spawners, etc.) is not reset.
    mutable std::unordered_map<std::uint64_t,std::shared_ptr<mc112::MineshaftStructure::Start>> mineshaftStarts_;
    mutable std::unordered_map<std::uint64_t,std::shared_ptr<mc112::VillageStructure::Start>> villageStarts_;
    mutable std::unordered_map<std::uint64_t,std::shared_ptr<mc112::StrongholdStructure::Start>> strongholdStarts_;
    mutable std::unordered_map<std::uint64_t,std::shared_ptr<mc112::ScatteredFeatureStructure::Start>> scatteredStarts_;
    mutable std::unordered_map<std::uint64_t,std::shared_ptr<mc112::OceanMonumentStructure::Start>> oceanMonumentStarts_;
    mutable std::unordered_map<std::uint64_t,std::shared_ptr<mc112::WoodlandMansionStructure::Start>> woodlandMansionStarts_;

    [[nodiscard]] bool isVillageChunk(int,int) const;
    [[nodiscard]] bool isTempleChunk(int,int) const;
    [[nodiscard]] bool isMineshaftChunk(int,int) const;
    [[nodiscard]] bool isStrongholdChunk(int,int) const;
    [[nodiscard]] bool isOceanMonumentChunk(int,int) const;
    [[nodiscard]] bool isWoodlandMansionChunk(int,int) const;
    [[nodiscard]] bool villageBiomeViable(int,int) const;
    [[nodiscard]] bool monumentBiomeViable(int,int) const;
    [[nodiscard]] bool mansionBiomeViable(int,int) const;
};
