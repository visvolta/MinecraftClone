#pragma once

#include "Block.h"
#include "core/ResourceLocation.h"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

using BiomeId = std::uint16_t;

enum class TreeFeature : std::uint8_t
{
    None,
    Oak,
    OakOnly,
    Plains,
    Forest,
    Birch,
    TallBirch,
    Spruce,
    Taiga,
    MegaTaiga,
    MegaSpruceTaiga,
    Jungle,
    JungleEdge,
    Savanna,
    RoofedForest,
    Hills,
    Swamp
};

enum class BiomeClimateCategory : std::uint8_t
{
    None,
    Warm,
    Medium,
    Cold,
    Ice
};

struct BiomeMobSpawn
{
    mc::core::ResourceLocation entity;
    int weight = 1;
    int minimumGroup = 1;
    int maximumGroup = 1;
};

struct BiomeDefinition
{
    BiomeId id = 0;
    mc::core::ResourceLocation name;
    std::string displayName;
    float baseHeight = 0.1f;
    float heightVariation = 0.2f;
    float temperature = 0.5f;
    float rainfall = 0.5f;
    bool snowy = false;
    bool rain = true;
    std::uint32_t waterColor = 0xFFFFFFU;
    std::optional<std::uint32_t> grassColor;
    std::optional<std::uint32_t> foliageColor;
    BlockType topBlock = BlockType::Grass;
    BlockType fillerBlock = BlockType::Dirt;
    TreeFeature treeFeature = TreeFeature::Oak;
    int treesPerChunk = 0;
    float extraTreeChance = 0.1f;
    int grassPerChunk = 1;
    int flowersPerChunk = 2;
    int deadBushesPerChunk = 0;
    int mushroomsPerChunk = 0;
    int reedsPerChunk = 0;
    int cactiPerChunk = 0;
    int bigMushroomsPerChunk = 0;
    int melonsPerChunk = 0;
    int vinesPerChunk = 0;
    int sandPatchesPerChunk = 3;
    int clayPatchesPerChunk = 1;
    int gravelPatchesPerChunk = 1;
    bool generateFalls = true;
    bool roofedForestDecoration = false;
    std::vector<BiomeMobSpawn> monsterSpawns;
    std::vector<BiomeMobSpawn> creatureSpawns;
    std::vector<BiomeMobSpawn> waterCreatureSpawns;
    std::vector<BiomeMobSpawn> ambientSpawns;
    std::array<int, 4> generationWeights{};
    std::optional<BiomeId> mutationOf;
};

class BiomeRegistry
{
public:
    void registerBiome(BiomeDefinition definition);
    void freeze();

    [[nodiscard]] const BiomeDefinition* find(BiomeId id) const noexcept;
    [[nodiscard]] BiomeDefinition* findMutable(BiomeId id) noexcept;
    [[nodiscard]] const BiomeDefinition* find(
        const mc::core::ResourceLocation& name) const noexcept;
    [[nodiscard]] BiomeId nextCustomId() const noexcept;
    [[nodiscard]] bool frozen() const noexcept;
    [[nodiscard]] const std::vector<BiomeDefinition>& entries() const noexcept;

    void activate() const noexcept;
    [[nodiscard]] static const BiomeRegistry& active() noexcept;
    [[nodiscard]] static const BiomeRegistry& vanilla();
    [[nodiscard]] static BiomeRegistry mutableVanilla();

private:
    std::vector<BiomeDefinition> entries_;
    std::unordered_map<BiomeId, std::size_t> byId_;
    std::unordered_map<
        mc::core::ResourceLocation,
        std::size_t,
        mc::core::ResourceLocationHash
    > byName_;
    bool frozen_ = false;
};

namespace VanillaBiomes
{
inline constexpr BiomeId Ocean = 0;
inline constexpr BiomeId Plains = 1;
inline constexpr BiomeId Desert = 2;
inline constexpr BiomeId ExtremeHills = 3;
inline constexpr BiomeId Forest = 4;
inline constexpr BiomeId Taiga = 5;
inline constexpr BiomeId Swampland = 6;
inline constexpr BiomeId River = 7;
inline constexpr BiomeId Hell = 8;
inline constexpr BiomeId Sky = 9;
inline constexpr BiomeId FrozenOcean = 10;
inline constexpr BiomeId FrozenRiver = 11;
inline constexpr BiomeId IcePlains = 12;
inline constexpr BiomeId IceMountains = 13;
inline constexpr BiomeId MushroomIsland = 14;
inline constexpr BiomeId MushroomShore = 15;
inline constexpr BiomeId Beach = 16;
inline constexpr BiomeId DesertHills = 17;
inline constexpr BiomeId ForestHills = 18;
inline constexpr BiomeId TaigaHills = 19;
inline constexpr BiomeId ExtremeHillsEdge = 20;
inline constexpr BiomeId Jungle = 21;
inline constexpr BiomeId JungleHills = 22;
inline constexpr BiomeId JungleEdge = 23;
inline constexpr BiomeId DeepOcean = 24;
inline constexpr BiomeId StoneBeach = 25;
inline constexpr BiomeId ColdBeach = 26;
inline constexpr BiomeId BirchForest = 27;
inline constexpr BiomeId BirchForestHills = 28;
inline constexpr BiomeId RoofedForest = 29;
inline constexpr BiomeId ColdTaiga = 30;
inline constexpr BiomeId ColdTaigaHills = 31;
inline constexpr BiomeId MegaTaiga = 32;
inline constexpr BiomeId MegaTaigaHills = 33;
inline constexpr BiomeId ExtremeHillsPlus = 34;
inline constexpr BiomeId Savanna = 35;
inline constexpr BiomeId SavannaPlateau = 36;
inline constexpr BiomeId Mesa = 37;
inline constexpr BiomeId MesaPlateauF = 38;
inline constexpr BiomeId MesaPlateau = 39;
inline constexpr BiomeId Void = 127;
inline constexpr BiomeId SunflowerPlains = 129;
inline constexpr BiomeId DesertMountains = 130;
inline constexpr BiomeId ExtremeHillsMountains = 131;
inline constexpr BiomeId FlowerForest = 132;
inline constexpr BiomeId TaigaMountains = 133;
inline constexpr BiomeId SwamplandMountains = 134;
inline constexpr BiomeId IcePlainsSpikes = 140;
inline constexpr BiomeId JungleMountains = 149;
inline constexpr BiomeId JungleEdgeMountains = 151;
inline constexpr BiomeId BirchForestMountains = 155;
inline constexpr BiomeId BirchForestHillsMountains = 156;
inline constexpr BiomeId RoofedForestMountains = 157;
inline constexpr BiomeId ColdTaigaMountains = 158;
inline constexpr BiomeId MegaSpruceTaiga = 160;
inline constexpr BiomeId MegaSpruceTaigaHills = 161;
inline constexpr BiomeId ExtremeHillsPlusMountains = 162;
inline constexpr BiomeId SavannaMountains = 163;
inline constexpr BiomeId SavannaPlateauMountains = 164;
inline constexpr BiomeId MesaBryce = 165;
inline constexpr BiomeId MesaPlateauFMountains = 166;
inline constexpr BiomeId MesaPlateauMountains = 167;
}

struct ClimateSample
{
    double temperature = 0.5;
    double humidity = 0.5;
    BiomeId biome = VanillaBiomes::Plains;
};

[[nodiscard]] BiomeId classifyReleaseBiome(
    double temperature,
    double humidity,
    double continentalness) noexcept;
[[nodiscard]] const char* biomeName(BiomeId biome) noexcept;
