#include "worldgen/StructureGenerator.h"

#include "Block.h"
#include "Chunk.h"
#include "worldgen/JavaRandom.h"
#include "worldgen/WorldGenerationContext.h"
#include "content/ContentCatalog.h"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <limits>
#include <numbers>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace
{
int floorDivide(int value, int divisor)
{
    int result = value / divisor;
    if (value % divisor < 0)
        --result;
    return result;
}

JavaRandom structureRandom(
    std::int64_t seed,
    int regionX,
    int regionZ,
    std::int64_t salt)
{
    return JavaRandom(
        static_cast<std::int64_t>(regionX) * 341873128712LL +
        static_cast<std::int64_t>(regionZ) * 132897987541LL +
        seed + salt
    );
}

std::pair<int, int> scatteredCandidate(
    std::int64_t seed,
    int regionX,
    int regionZ,
    int spacing,
    int separation,
    std::int64_t salt)
{
    JavaRandom random = structureRandom(seed, regionX, regionZ, salt);
    return {
        regionX * spacing + random.nextInt(spacing - separation),
        regionZ * spacing + random.nextInt(spacing - separation)
    };
}

std::pair<int, int> triangularCandidate(
    std::int64_t seed,
    int regionX,
    int regionZ,
    int spacing,
    int separation,
    std::int64_t salt)
{
    JavaRandom random = structureRandom(seed, regionX, regionZ, salt);
    const int bound = spacing - separation;
    return {
        regionX * spacing + (random.nextInt(bound) + random.nextInt(bound)) / 2,
        regionZ * spacing + (random.nextInt(bound) + random.nextInt(bound)) / 2
    };
}

bool villageBiome(BiomeId biome)
{
    return biome == VanillaBiomes::Plains ||
           biome == VanillaBiomes::Desert ||
           biome == VanillaBiomes::Savanna ||
           biome == VanillaBiomes::Taiga;
}

bool templeBiome(BiomeId biome)
{
    return biome == VanillaBiomes::Desert ||
           biome == VanillaBiomes::DesertHills ||
           biome == VanillaBiomes::Jungle ||
           biome == VanillaBiomes::JungleHills ||
           biome == VanillaBiomes::Swampland ||
           biome == VanillaBiomes::SwamplandMountains ||
           biome == VanillaBiomes::IcePlains ||
           biome == VanillaBiomes::ColdTaiga;
}

bool monumentBiome(BiomeId biome)
{
    return biome == VanillaBiomes::DeepOcean;
}

bool mansionBiome(BiomeId biome)
{
    return biome == VanillaBiomes::RoofedForest ||
           biome == VanillaBiomes::RoofedForestMountains;
}

mc::content::BlockState registeredState(
    std::string_view name,
    BlockType fallback)
{
    const mc::content::ContentCatalog* catalog =
        mc::content::ContentCatalog::active();
    if (catalog != nullptr)
    {
        const auto state = catalog->state(
            mc::core::ResourceLocation("minecraft", name)
        );
        if (state)
            return *state;
    }
    return mc::content::BlockState(fallback);
}

mc::content::BlockState registeredState(
    std::string_view name,
    std::initializer_list<std::pair<std::string, std::string>> properties,
    BlockType fallback)
{
    const mc::content::ContentCatalog* catalog =
        mc::content::ContentCatalog::active();
    if (catalog != nullptr)
    {
        const std::vector<std::pair<std::string, std::string>> values(properties);
        const auto state = catalog->state(
            mc::core::ResourceLocation("minecraft", name), values
        );
        if (state)
            return *state;
    }
    return mc::content::BlockState(fallback);
}

void fill(
    WorldGenerationContext& context,
    int minX, int minY, int minZ,
    int maxX, int maxY, int maxZ,
    BlockType block)
{
    for (int x = minX; x <= maxX; ++x)
        for (int y = minY; y <= maxY; ++y)
            for (int z = minZ; z <= maxZ; ++z)
                context.setBlock(x, y, z, block);
}

void fillState(
    WorldGenerationContext& context,
    int minX, int minY, int minZ,
    int maxX, int maxY, int maxZ,
    mc::content::BlockState state)
{
    for (int x = minX; x <= maxX; ++x)
        for (int y = minY; y <= maxY; ++y)
            for (int z = minZ; z <= maxZ; ++z)
                context.setBlockState(x, y, z, state);
}

void hollowBox(
    WorldGenerationContext& context,
    int minX, int minY, int minZ,
    int maxX, int maxY, int maxZ,
    BlockType wall)
{
    for (int x = minX; x <= maxX; ++x)
        for (int y = minY; y <= maxY; ++y)
            for (int z = minZ; z <= maxZ; ++z)
                context.setBlock(
                    x, y, z,
                    x == minX || x == maxX || y == minY || y == maxY ||
                    z == minZ || z == maxZ ? wall : BlockType::Air);
}

void generateVillageWoodHut(
    WorldGenerationContext& context,
    int x,
    int y,
    int z,
    BlockType planks,
    BlockType logs)
{
    // StructureVillagePieces.WoodHut is a 4x6x5 piece.
    fill(context, x, y - 1, z, x + 3, y - 1, z + 4, BlockType::Cobblestone);
    fill(context, x + 1, y - 1, z + 1, x + 2, y - 1, z + 3, BlockType::Dirt);
    hollowBox(context, x, y, z, x + 3, y + 3, z + 4, planks);
    for (const auto& [dx, dz] : {
             std::pair{0, 0}, std::pair{3, 0},
             std::pair{0, 4}, std::pair{3, 4}})
        fill(context, x + dx, y, z + dz, x + dx, y + 3, z + dz, logs);
    fill(context, x, y + 4, z + 1, x + 3, y + 4, z + 3, logs);
    context.setBlock(x + 1, y + 1, z, BlockType::Air);
    context.setBlock(x + 1, y + 2, z, BlockType::Air);
    context.setBlockState(x, y + 1, z + 2,
        registeredState("glass_pane", BlockType::Glass));
    context.setBlockState(x + 3, y + 1, z + 2,
        registeredState("glass_pane", BlockType::Glass));
}

void generateVillageHouse(
    WorldGenerationContext& context,
    int x,
    int y,
    int z,
    BlockType planks,
    std::string_view stairName)
{
    // StructureVillagePieces.House1 footprint and roof profile: 9x9x6.
    fill(context, x, y - 1, z, x + 8, y - 1, z + 5, BlockType::Cobblestone);
    hollowBox(context, x, y, z, x + 8, y + 4, z + 5, planks);
    fill(context, x, y, z, x, y + 3, z + 5, BlockType::Cobblestone);
    fill(context, x + 8, y, z, x + 8, y + 3, z + 5, BlockType::Cobblestone);
    const auto northStair = registeredState(
        stairName,
        {{"facing", "north"}, {"half", "bottom"}, {"shape", "straight"}},
        planks
    );
    const auto southStair = registeredState(
        stairName,
        {{"facing", "south"}, {"half", "bottom"}, {"shape", "straight"}},
        planks
    );
    for (int slope = -1; slope <= 2; ++slope)
    {
        for (int dx = 0; dx <= 8; ++dx)
        {
            context.setBlockState(x + dx, y + 5 + slope, z + slope, northStair);
            context.setBlockState(x + dx, y + 5 + slope, z + 5 - slope, southStair);
        }
    }
    const auto pane = registeredState("glass_pane", BlockType::Glass);
    for (const auto& [dx, dz] : {
             std::pair{4, 0}, std::pair{5, 0}, std::pair{6, 0},
             std::pair{2, 5}, std::pair{3, 5}, std::pair{5, 5},
             std::pair{6, 5}})
        context.setBlockState(x + dx, y + 2, z + dz, pane);
    context.setBlock(x + 1, y + 1, z, BlockType::Air);
    context.setBlock(x + 1, y + 2, z, BlockType::Air);
    context.setBlock(x + 7, y, z + 1, BlockType::CraftingTable);
    fill(context, x + 1, y + 3, z + 4, x + 7, y + 3, z + 4,
         BlockType::Bookshelf);
}

void generateVillageChurch(
    WorldGenerationContext& context,
    int x,
    int y,
    int z)
{
    // StructureVillagePieces.Church is the distinctive 5x12x9 cobblestone
    // piece with a 5x5, 11-block-high bell tower.
    fill(context, x + 1, y - 1, z, x + 3, y - 1, z + 8,
         BlockType::Cobblestone);
    hollowBox(context, x, y, z, x + 4, y + 4, z + 8,
              BlockType::Cobblestone);
    hollowBox(context, x, y + 4, z, x + 4, y + 9, z + 4,
              BlockType::Cobblestone);
    fill(context, x, y + 9, z, x + 4, y + 9, z + 4,
         BlockType::Cobblestone);
    context.setBlock(x + 2, y + 10, z, BlockType::Cobblestone);
    context.setBlock(x + 2, y + 10, z + 4, BlockType::Cobblestone);
    context.setBlock(x, y + 10, z + 2, BlockType::Cobblestone);
    context.setBlock(x + 4, y + 10, z + 2, BlockType::Cobblestone);
    const auto pane = registeredState("glass_pane", BlockType::Glass);
    for (const auto& [dx, dy, dz] : {
             std::tuple{0, 2, 2}, std::tuple{4, 2, 2},
             std::tuple{0, 6, 2}, std::tuple{4, 6, 2},
             std::tuple{2, 6, 0}, std::tuple{2, 6, 4},
             std::tuple{0, 3, 6}, std::tuple{4, 3, 6},
             std::tuple{2, 3, 8}})
        context.setBlockState(x + dx, y + dy, z + dz, pane);
    context.setBlock(x + 2, y + 1, z, BlockType::Air);
    context.setBlock(x + 2, y + 2, z, BlockType::Air);
    for (int dy = 1; dy <= 8; ++dy)
        context.setBlock(x + 3, y + dy, z + 3, BlockType::Ladder);
}

void generateVillageField(
    WorldGenerationContext& context,
    int x,
    int y,
    int z,
    JavaRandom& random)
{
    // The two 1.12 farm pieces use irrigated rows enclosed by logs.
    fill(context, x, y - 1, z, x + 12, y - 1, z + 8, BlockType::Dirt);
    for (int dx = 0; dx <= 12; ++dx)
    {
        context.setBlock(x + dx, y, z, BlockType::OakLog);
        context.setBlock(x + dx, y, z + 8, BlockType::OakLog);
    }
    for (int dz = 1; dz < 8; ++dz)
    {
        context.setBlock(x, y, z + dz, BlockType::OakLog);
        context.setBlock(x + 12, y, z + dz, BlockType::OakLog);
        context.setBlock(x + 4, y, z + dz, BlockType::Water);
        context.setBlock(x + 8, y, z + dz, BlockType::Water);
        for (int dx = 1; dx < 12; ++dx)
        {
            if (dx == 4 || dx == 8)
                continue;
            context.setBlockState(
                x + dx, y, z + dz,
                mc::content::BlockState(BlockType::Farmland, 7)
            );
            context.setBlockState(
                x + dx, y + 1, z + dz,
                mc::content::BlockState(
                    BlockType::Wheat,
                    static_cast<std::uint8_t>(2 + random.nextInt(6))
                )
            );
        }
    }
}
}

const char* structureName(WorldStructure structure) noexcept
{
    switch (structure)
    {
        case WorldStructure::Mineshaft: return "Mineshaft";
        case WorldStructure::Village: return "Village";
        case WorldStructure::Temple: return "Temple";
        case WorldStructure::Stronghold: return "Stronghold";
        case WorldStructure::OceanMonument: return "Monument";
        case WorldStructure::WoodlandMansion: return "Mansion";
    }
    return "Unknown";
}

std::optional<WorldStructure> parseStructureName(std::string_view name) noexcept
{
    std::string normalized(name);
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
        [](unsigned char character)
        {
            return static_cast<char>(std::tolower(character));
        });
    if (normalized == "mineshaft") return WorldStructure::Mineshaft;
    if (normalized == "village") return WorldStructure::Village;
    if (normalized == "temple" || normalized == "pyramid")
        return WorldStructure::Temple;
    if (normalized == "stronghold") return WorldStructure::Stronghold;
    if (normalized == "monument" || normalized == "ocean_monument")
        return WorldStructure::OceanMonument;
    if (normalized == "mansion" || normalized == "woodland_mansion")
        return WorldStructure::WoodlandMansion;
    return std::nullopt;
}

StructureGenerator::StructureGenerator(std::int64_t worldSeed)
    : worldSeed_(worldSeed)
{
    // MapGenStronghold 1.12 defaults: 128 positions, distance 32, spread 3.
    // Biome relocation is intentionally kept out of this seed phase so the
    // ring is stable and can later be adjusted by a biome-provider query.
    JavaRandom random(worldSeed_);
    double angle = random.nextDouble() * std::numbers::pi * 2.0;
    int ring = 0;
    int inRing = 0;
    int spread = 3;
    strongholdChunks_.reserve(128);
    for (int index = 0; index < 128; ++index)
    {
        const double distance = 4.0 * 32.0 + 32.0 * ring * 6.0 +
            (random.nextDouble() - 0.5) * 32.0 * 2.5;
        strongholdChunks_.emplace_back(
            static_cast<int>(std::floor(std::cos(angle) * distance + 0.5)),
            static_cast<int>(std::floor(std::sin(angle) * distance + 0.5))
        );
        angle += std::numbers::pi * 2.0 / spread;
        if (++inRing == spread)
        {
            ++ring;
            inRing = 0;
            spread += 2 * spread / (ring + 1);
            spread = std::min(spread, 128 - index);
            angle += random.nextDouble() * std::numbers::pi * 2.0;
        }
    }
}

bool StructureGenerator::isVillageChunk(int chunkX, int chunkZ) const
{
    const int regionX = floorDivide(chunkX, 32);
    const int regionZ = floorDivide(chunkZ, 32);
    return scatteredCandidate(worldSeed_, regionX, regionZ, 32, 8, 10387312LL) ==
        std::pair{chunkX, chunkZ};
}

bool StructureGenerator::isTempleChunk(int chunkX, int chunkZ) const
{
    const int regionX = floorDivide(chunkX, 32);
    const int regionZ = floorDivide(chunkZ, 32);
    return scatteredCandidate(worldSeed_, regionX, regionZ, 32, 8, 14357617LL) ==
        std::pair{chunkX, chunkZ};
}

bool StructureGenerator::isMineshaftChunk(int chunkX, int chunkZ) const
{
    JavaRandom random(
        worldSeed_ + static_cast<std::int64_t>(chunkX) * 341873128712LL +
        static_cast<std::int64_t>(chunkZ) * 132897987541LL);
    return random.nextDouble() < 0.004 &&
        random.nextInt(80) < std::max(std::abs(chunkX), std::abs(chunkZ));
}

bool StructureGenerator::isStrongholdChunk(int chunkX, int chunkZ) const
{
    return std::find(
        strongholdChunks_.begin(), strongholdChunks_.end(),
        std::pair{chunkX, chunkZ}
    ) != strongholdChunks_.end();
}

bool StructureGenerator::isOceanMonumentChunk(int chunkX, int chunkZ) const
{
    const int regionX = floorDivide(chunkX, 32);
    const int regionZ = floorDivide(chunkZ, 32);
    return triangularCandidate(
        worldSeed_, regionX, regionZ, 32, 5, 10387313LL
    ) == std::pair{chunkX, chunkZ};
}

bool StructureGenerator::isWoodlandMansionChunk(int chunkX, int chunkZ) const
{
    const int regionX = floorDivide(chunkX, 80);
    const int regionZ = floorDivide(chunkZ, 80);
    return triangularCandidate(
        worldSeed_, regionX, regionZ, 80, 20, 10387319LL
    ) == std::pair{chunkX, chunkZ};
}

void StructureGenerator::populate(
    Chunk& targetChunk,
    WorldGenerationContext& context) const
{
    for (int sourceX = targetChunk.getChunkX() - 4;
         sourceX <= targetChunk.getChunkX() + 4; ++sourceX)
    {
        for (int sourceZ = targetChunk.getChunkZ() - 4;
             sourceZ <= targetChunk.getChunkZ() + 4; ++sourceZ)
        {
            const ClimateSample climate = context.sampleClimate(
                sourceX * 16 + 8, sourceZ * 16 + 8);
            if (isMineshaftChunk(sourceX, sourceZ))
            {
                JavaRandom random = structureRandom(
                    worldSeed_, sourceX, sourceZ, 0x4D494E45LL);
                context.beginIsolatedFeature();
                generateMineshaft(context, random, sourceX, sourceZ);
                context.finishIsolatedFeature(true);
            }
            if (isVillageChunk(sourceX, sourceZ) && villageBiome(climate.biome))
            {
                JavaRandom random = structureRandom(
                    worldSeed_, sourceX, sourceZ, 10387312LL);
                context.beginIsolatedFeature();
                generateVillage(context, random, sourceX, sourceZ, climate.biome);
                context.finishIsolatedFeature(true);
            }
            if (isTempleChunk(sourceX, sourceZ) && templeBiome(climate.biome))
            {
                JavaRandom random = structureRandom(
                    worldSeed_, sourceX, sourceZ, 14357617LL);
                context.beginIsolatedFeature();
                generateTemple(context, random, sourceX, sourceZ, climate.biome);
                context.finishIsolatedFeature(true);
            }
            if (isStrongholdChunk(sourceX, sourceZ))
            {
                JavaRandom random = structureRandom(
                    worldSeed_, sourceX, sourceZ, 0x5354524F4E47LL);
                context.beginIsolatedFeature();
                generateStronghold(context, random, sourceX, sourceZ);
                context.finishIsolatedFeature(true);
            }
            if (isOceanMonumentChunk(sourceX, sourceZ) &&
                monumentBiome(climate.biome))
            {
                JavaRandom random = structureRandom(
                    worldSeed_, sourceX, sourceZ, 10387313LL);
                context.beginIsolatedFeature();
                generateOceanMonument(context, random, sourceX, sourceZ);
                context.finishIsolatedFeature(true);
            }
            if (isWoodlandMansionChunk(sourceX, sourceZ) &&
                mansionBiome(climate.biome))
            {
                JavaRandom random = structureRandom(
                    worldSeed_, sourceX, sourceZ, 10387319LL);
                context.beginIsolatedFeature();
                generateWoodlandMansion(context, random, sourceX, sourceZ);
                context.finishIsolatedFeature(true);
            }
        }
    }
}

void StructureGenerator::generateMineshaft(
    WorldGenerationContext& context,
    JavaRandom& random,
    int chunkX,
    int chunkZ) const
{
    const int centreX = chunkX * 16 + 8;
    const int centreZ = chunkZ * 16 + 8;
    const int y = 18 + random.nextInt(28);
    hollowBox(context, centreX - 4, y - 1, centreZ - 4,
              centreX + 4, y + 3, centreZ + 4, BlockType::OakPlanks);
    for (int direction = 0; direction < 4; ++direction)
    {
        const int dx = direction == 0 ? 1 : direction == 1 ? -1 : 0;
        const int dz = direction == 2 ? 1 : direction == 3 ? -1 : 0;
        const int length = 18 + random.nextInt(23);
        for (int step = 4; step <= length; ++step)
        {
            const int x = centreX + dx * step;
            const int z = centreZ + dz * step;
            fill(context, x - (dz != 0), y, z - (dx != 0),
                  x + (dz != 0), y + 2, z + (dx != 0), BlockType::Air);
            if (step % 5 == 0)
            {
                if (dz != 0)
                {
                    fill(context, x - 2, y, z, x - 2, y + 2, z, BlockType::OakLog);
                    fill(context, x + 2, y, z, x + 2, y + 2, z, BlockType::OakLog);
                    fill(context, x - 2, y + 2, z, x + 2, y + 2, z, BlockType::OakPlanks);
                }
                else
                {
                    fill(context, x, y, z - 2, x, y + 2, z - 2, BlockType::OakLog);
                    fill(context, x, y, z + 2, x, y + 2, z + 2, BlockType::OakLog);
                    fill(context, x, y + 2, z - 2, x, y + 2, z + 2, BlockType::OakPlanks);
                }
            }
            if (random.nextInt(42) == 0)
                context.setBlock(x, y + 1, z, BlockType::Cobweb);
        }
    }
    context.setBlock(centreX + 2, y, centreZ + 2, BlockType::Chest);
}

void StructureGenerator::generateVillage(
    WorldGenerationContext& context,
    JavaRandom& random,
    int chunkX,
    int chunkZ,
    BiomeId biome) const
{
    const int centreX = chunkX * 16 + 8;
    const int centreZ = chunkZ * 16 + 8;
    const int groundY = context.getHeightValue(centreX, centreZ);
    const BlockType planks = biome == VanillaBiomes::Taiga
        ? BlockType::SprucePlanks
        : biome == VanillaBiomes::Savanna
            ? BlockType::AcaciaPlanks : BlockType::OakPlanks;
    const BlockType wall = biome == VanillaBiomes::Desert
        ? BlockType::Sandstone : planks;
    const BlockType logs = biome == VanillaBiomes::Taiga
        ? BlockType::SpruceLog
        : biome == VanillaBiomes::Savanna
            ? BlockType::AcaciaLog : BlockType::OakLog;
    const std::string_view stairName = biome == VanillaBiomes::Desert
        ? "sandstone_stairs"
        : biome == VanillaBiomes::Taiga
            ? "spruce_stairs"
            : biome == VanillaBiomes::Savanna
                ? "acacia_stairs" : "oak_stairs";
    const BlockType road = biome == VanillaBiomes::Desert
        ? BlockType::Sandstone : BlockType::Gravel;

    // The well is the vanilla village start piece; roads and houses branch
    // from its four sides using the same seeded candidate chunk.
    fill(context, centreX - 2, groundY - 1, centreZ - 2,
         centreX + 2, groundY, centreZ + 2, BlockType::Cobblestone);
    fill(context, centreX - 1, groundY, centreZ - 1,
         centreX + 1, groundY + 2, centreZ + 1, BlockType::Water);
    for (int y = groundY + 1; y <= groundY + 4; ++y)
        for (const auto& [dx, dz] : {std::pair{-2, -2}, std::pair{2, -2},
                                     std::pair{-2, 2}, std::pair{2, 2}})
            context.setBlock(centreX + dx, y, centreZ + dz, wall);
    fill(context, centreX - 2, groundY + 4, centreZ - 2,
         centreX + 2, groundY + 4, centreZ + 2, wall);

    for (int offset = -28; offset <= 28; ++offset)
    {
        const int xY = context.getHeightValue(centreX + offset, centreZ);
        const int zY = context.getHeightValue(centreX, centreZ + offset);
        context.setBlock(centreX + offset, xY - 1, centreZ, road);
        context.setBlock(centreX, zY - 1, centreZ + offset, road);
    }
    const int pieceCount = 5 + random.nextInt(5);
    for (int piece = 0; piece < pieceCount; ++piece)
    {
        const bool alongX = piece % 2 == 0;
        const int side = piece % 4 < 2 ? -1 : 1;
        const int distance = 10 + (piece / 4) * 14 + random.nextInt(5);
        const int x = centreX + (alongX ? side * distance : side * 6);
        const int z = centreZ + (alongX ? side * 6 : side * distance);
        const int y = context.getHeightValue(x, z);

        // StructureVillagePieces uses these exact relative selection weights:
        // garden 4, church 20, house 20, hut 3, hall 15, fields 3+3,
        // small house 15, butcher 8 (total 91).
        const int selection = random.nextInt(91);
        if (selection < 4)
        {
            generateVillageHouse(context, x - 4, y, z - 3, planks, stairName);
        }
        else if (selection < 24)
        {
            generateVillageChurch(context, x - 2, y, z - 4);
        }
        else if (selection < 44)
        {
            generateVillageHouse(context, x - 4, y, z - 3, planks, stairName);
        }
        else if (selection < 47)
        {
            generateVillageWoodHut(context, x - 2, y, z - 2, planks, logs);
        }
        else if (selection < 62)
        {
            generateVillageHouse(context, x - 4, y, z - 3, wall, stairName);
        }
        else if (selection < 68)
        {
            generateVillageField(context, x - 6, y, z - 4, random);
        }
        else
        {
            generateVillageHouse(context, x - 4, y, z - 3, wall, stairName);
        }
        if (piece == 0)
            context.setBlock(x + 1, y, z + 1, BlockType::Chest);
    }
}

void StructureGenerator::generateTemple(
    WorldGenerationContext& context,
    JavaRandom&,
    int chunkX,
    int chunkZ,
    BiomeId biome) const
{
    const int x = chunkX * 16;
    const int z = chunkZ * 16;
    const int y = context.getHeightValue(x + 8, z + 8);
    if (biome == VanillaBiomes::Desert || biome == VanillaBiomes::DesertHills)
    {
        for (int layer = 0; layer < 4; ++layer)
            fill(context, x - 2 + layer, y + layer, z - 2 + layer,
                 x + 18 - layer, y + layer, z + 18 - layer,
                 BlockType::Sandstone);
        hollowBox(context, x + 3, y + 1, z + 3,
                  x + 13, y + 8, z + 13, BlockType::Sandstone);
        fill(context, x + 7, y, z + 7, x + 9, y, z + 9,
             BlockType::OrangeWool);
        context.setBlock(x + 8, y, z + 8, BlockType::BlueWool);
        fill(context, x + 8, y - 10, z + 8, x + 8, y - 1, z + 8,
             BlockType::Air);
        fill(context, x + 7, y - 11, z + 7, x + 9, y - 11, z + 9,
             BlockType::Sandstone);
        context.setBlock(x + 8, y - 10, z + 8, BlockType::TNT);
        for (const auto& [dx, dz] : {std::pair{-1, 0}, std::pair{1, 0},
                                     std::pair{0, -1}, std::pair{0, 1}})
            context.setBlock(x + 8 + dx, y - 10, z + 8 + dz, BlockType::Chest);
    }
    else if (biome == VanillaBiomes::Jungle ||
             biome == VanillaBiomes::JungleHills)
    {
        hollowBox(context, x + 2, y, z + 1,
                  x + 13, y + 7, z + 15, BlockType::MossyCobblestone);
        fill(context, x + 1, y + 7, z, x + 14, y + 7, z + 16,
             BlockType::Cobblestone);
        fill(context, x + 4, y - 4, z + 4, x + 11, y - 1, z + 12,
             BlockType::MossyCobblestone);
        hollowBox(context, x + 5, y - 3, z + 5,
                  x + 10, y, z + 11, BlockType::StoneBricks);
        context.setBlock(x + 6, y - 2, z + 9, BlockType::Chest);
        context.setBlock(x + 9, y - 2, z + 7, BlockType::Chest);
        context.setBlock(x + 7, y + 2, z + 1, BlockType::Air);
        context.setBlock(x + 8, y + 2, z + 1, BlockType::Air);
    }
    else if (biome == VanillaBiomes::Swampland ||
             biome == VanillaBiomes::SwamplandMountains)
    {
        // 1.12 swamp huts stand on four oak-log stilts with a compact plank
        // room, overhanging roof, crafting table, and single entrance.
        const int floorY = y + 3;
        for (const auto& [dx, dz] : {
                 std::pair{2, 2}, std::pair{8, 2},
                 std::pair{2, 8}, std::pair{8, 8}})
            fill(context, x + dx, y - 3, z + dz,
                 x + dx, floorY + 2, z + dz, BlockType::OakLog);
        fill(context, x + 2, floorY, z + 2,
             x + 8, floorY, z + 8, BlockType::OakPlanks);
        hollowBox(context, x + 2, floorY + 1, z + 2,
                  x + 8, floorY + 4, z + 8, BlockType::OakPlanks);
        fill(context, x + 1, floorY + 4, z + 1,
             x + 9, floorY + 4, z + 9, BlockType::SprucePlanks);
        context.setBlock(x + 5, floorY + 1, z + 2, BlockType::Air);
        context.setBlock(x + 7, floorY + 1, z + 7, BlockType::CraftingTable);
    }
    else
    {
        // Igloo shell and tunnel. Use the registry snow block rather than the
        // legacy snow layer so vertical walls have full-cube collision.
        const mc::content::BlockState snow = registeredState(
            "snow", BlockType::WhiteWool
        );
        for (int dx = -4; dx <= 4; ++dx)
        {
            for (int dz = -4; dz <= 4; ++dz)
            {
                const int distance = dx * dx + dz * dz;
                if (distance <= 16)
                    context.setBlockState(x + 8 + dx, y, z + 8 + dz, snow);
                if (distance >= 9 && distance <= 16)
                    context.setBlockState(x + 8 + dx, y + 1, z + 8 + dz, snow);
                if (distance >= 4 && distance <= 10)
                    context.setBlockState(x + 8 + dx, y + 2, z + 8 + dz, snow);
            }
        }
        fillState(context, x + 7, y + 1, z + 4,
                  x + 9, y + 2, z + 7, snow);
        fill(context, x + 8, y + 1, z + 4,
             x + 8, y + 2, z + 8, BlockType::Air);
        context.setBlock(x + 6, y + 1, z + 8, BlockType::Furnace);
        context.setBlock(x + 10, y + 1, z + 8, BlockType::CraftingTable);
    }
}

void StructureGenerator::generateStronghold(
    WorldGenerationContext& context,
    JavaRandom& random,
    int chunkX,
    int chunkZ) const
{
    const int centreX = chunkX * 16 + 2;
    const int centreZ = chunkZ * 16 + 2;
    const int y = 20 + random.nextInt(20);
    const auto bricks = registeredState(
        "stonebrick", BlockType::StoneBricks
    );
    const auto mossy = registeredState(
        "mossy_stonebrick", BlockType::MossyCobblestone
    );
    const auto frame = registeredState("end_portal_frame", BlockType::StoneBricks);

    // A deterministic connected start, crossing, library and portal room.
    // The bounding layout follows the vanilla stronghold's underground scale
    // while every room remains independently replayable across chunk borders.
    fillState(context, centreX - 4, y - 1, centreZ - 4,
              centreX + 4, y + 4, centreZ + 4, bricks);
    fill(context, centreX - 3, y, centreZ - 3,
         centreX + 3, y + 3, centreZ + 3, BlockType::Air);
    for (int arm = 0; arm < 4; ++arm)
    {
        const int dx = arm == 0 ? 1 : arm == 1 ? -1 : 0;
        const int dz = arm == 2 ? 1 : arm == 3 ? -1 : 0;
        const int length = 18 + random.nextInt(20);
        for (int step = 4; step <= length; ++step)
        {
            const int x = centreX + dx * step;
            const int z = centreZ + dz * step;
            fillState(context, x - (dz != 0) * 2, y - 1,
                      z - (dx != 0) * 2, x + (dz != 0) * 2, y + 3,
                      z + (dx != 0) * 2,
                      step % 9 == 0 ? mossy : bricks);
            fill(context, x - (dz != 0), y, z - (dx != 0),
                 x + (dz != 0), y + 2, z + (dx != 0), BlockType::Air);
        }
    }

    const int portalX = centreX + 28;
    hollowBox(context, portalX - 5, y - 2, centreZ - 5,
              portalX + 5, y + 5, centreZ + 5, BlockType::StoneBricks);
    fill(context, portalX - 4, y - 1, centreZ - 4,
         portalX + 4, y + 4, centreZ + 4, BlockType::Air);
    fill(context, portalX - 2, y - 1, centreZ - 1,
         portalX + 2, y - 1, centreZ + 1, BlockType::Lava);
    for (int offset = -2; offset <= 2; ++offset)
    {
        context.setBlockState(portalX + offset, y, centreZ - 2, frame);
        context.setBlockState(portalX + offset, y, centreZ + 2, frame);
        context.setBlockState(portalX - 3, y, centreZ + offset, frame);
        context.setBlockState(portalX + 3, y, centreZ + offset, frame);
    }
    context.setBlock(portalX - 4, y, centreZ, BlockType::Spawner);

    const int libraryX = centreX - 25;
    hollowBox(context, libraryX - 7, y - 1, centreZ - 5,
              libraryX + 7, y + 6, centreZ + 5, BlockType::StoneBricks);
    fill(context, libraryX - 6, y, centreZ - 4,
         libraryX + 6, y + 5, centreZ + 4, BlockType::Air);
    for (int dx = -5; dx <= 5; dx += 2)
    {
        fill(context, libraryX + dx, y, centreZ - 4,
             libraryX + dx, y + 4, centreZ - 4, BlockType::Bookshelf);
        fill(context, libraryX + dx, y, centreZ + 4,
             libraryX + dx, y + 4, centreZ + 4, BlockType::Bookshelf);
    }
    context.setBlock(libraryX, y, centreZ, BlockType::Chest);
}

void StructureGenerator::generateOceanMonument(
    WorldGenerationContext& context,
    JavaRandom&,
    int chunkX,
    int chunkZ) const
{
    const int x = chunkX * 16 - 21;
    const int z = chunkZ * 16 - 21;
    constexpr int y = 39;
    const auto prismarine = registeredState("prismarine", BlockType::StoneBricks);
    const auto bricks = registeredState(
        "prismarine_bricks", BlockType::StoneBricks
    );
    const auto dark = registeredState(
        "dark_prismarine", BlockType::StoneBricks
    );
    const auto lamp = registeredState("sea_lantern", BlockType::Glowstone);

    fillState(context, x, y, z, x + 57, y + 2, z + 57, bricks);
    for (int tier = 0; tier < 4; ++tier)
    {
        const int inset = tier * 6;
        const int top = y + 8 + tier * 5;
        fillState(context, x + inset, y + 3, z + inset,
                  x + 57 - inset, top, z + 57 - inset, prismarine);
        fill(context, x + inset + 2, y + 4, z + inset + 2,
             x + 55 - inset, top - 1, z + 55 - inset, BlockType::Water);
    }
    fillState(context, x + 23, y + 3, z + 23,
              x + 34, y + 22, z + 34, dark);
    fill(context, x + 25, y + 4, z + 25,
         x + 32, y + 20, z + 32, BlockType::Water);
    for (const auto& [dx, dz] : {
             std::pair{8, 8}, std::pair{49, 8},
             std::pair{8, 49}, std::pair{49, 49},
             std::pair{28, 4}, std::pair{28, 53}})
        context.setBlockState(x + dx, y + 8, z + dz, lamp);
    fill(context, x + 27, y + 4, z, x + 30, y + 9, z + 8, BlockType::Water);
}

void StructureGenerator::generateWoodlandMansion(
    WorldGenerationContext& context,
    JavaRandom& random,
    int chunkX,
    int chunkZ) const
{
    const int x = chunkX * 16 - 16;
    const int z = chunkZ * 16 - 12;
    const int y = context.getHeightValue(chunkX * 16 + 8, chunkZ * 16 + 8);
    const auto darkPlanks = registeredState("dark_oak_planks", BlockType::DarkOakPlanks);
    const auto darkLog = registeredState("dark_oak_log", BlockType::DarkOakLog);
    const auto pane = registeredState("glass_pane", BlockType::Glass);

    fill(context, x, y - 2, z, x + 39, y - 1, z + 29, BlockType::Cobblestone);
    for (int floor = 0; floor < 3; ++floor)
    {
        const int floorY = y + floor * 7;
        fillState(context, x, floorY, z, x + 39, floorY, z + 29, darkPlanks);
        for (int wallY = floorY + 1; wallY <= floorY + 6; ++wallY)
        {
            for (int dx = 0; dx <= 39; ++dx)
            {
                context.setBlockState(x + dx, wallY, z, darkPlanks);
                context.setBlockState(x + dx, wallY, z + 29, darkPlanks);
            }
            for (int dz = 1; dz < 29; ++dz)
            {
                context.setBlockState(x, wallY, z + dz, darkPlanks);
                context.setBlockState(x + 39, wallY, z + dz, darkPlanks);
            }
        }
        for (int roomX = 10; roomX < 39; roomX += 10)
            fillState(context, x + roomX, floorY + 1, z + 1,
                      x + roomX, floorY + 5, z + 28, darkPlanks);
        for (int roomZ = 10; roomZ < 29; roomZ += 10)
            fillState(context, x + 1, floorY + 1, z + roomZ,
                      x + 38, floorY + 5, z + roomZ, darkPlanks);
        for (int window = 4; window < 38; window += 6)
        {
            context.setBlockState(x + window, floorY + 3, z, pane);
            context.setBlockState(x + window, floorY + 3, z + 29, pane);
        }
        for (const auto& [dx, dz] : {
                 std::pair{0, 0}, std::pair{39, 0},
                 std::pair{0, 29}, std::pair{39, 29}})
            fillState(context, x + dx, floorY + 1, z + dz,
                      x + dx, floorY + 6, z + dz, darkLog);
        context.setBlock(x + 20, floorY + 1, z + 15, BlockType::Chest);
        if (random.nextBoolean())
            fill(context, x + 4, floorY + 1, z + 4,
                 x + 8, floorY + 4, z + 4, BlockType::Bookshelf);
    }
    for (int inset = 0; inset < 5; ++inset)
        fillState(context, x - 2 + inset, y + 21 + inset, z - 2 + inset,
                  x + 41 - inset, y + 21 + inset, z + 31 - inset, darkPlanks);
    fill(context, x + 18, y + 1, z, x + 21, y + 5, z, BlockType::Air);
}

std::optional<StructureLocation> StructureGenerator::findNearest(
    WorldStructure structure,
    int blockX,
    int blockZ,
    int maximumRegionRadius,
    const ClimateSampler& climateSampler) const
{
    const int originChunkX = floorDivide(blockX, 16);
    const int originChunkZ = floorDivide(blockZ, 16);
    double closestDistance = std::numeric_limits<double>::max();
    std::optional<StructureLocation> closest;
    if (structure == WorldStructure::Stronghold)
    {
        for (const auto& [chunkX, chunkZ] : strongholdChunks_)
        {
            const int x = chunkX * 16 + 8;
            const int z = chunkZ * 16 + 8;
            const double distance = std::hypot(
                static_cast<double>(x - blockX),
                static_cast<double>(z - blockZ)
            );
            if (distance < closestDistance)
            {
                closestDistance = distance;
                closest = StructureLocation{
                    structure, x, z, climateSampler(x, z).biome
                };
            }
        }
        return closest;
    }
    const int spacing = structure == WorldStructure::Mineshaft ? 1 :
        structure == WorldStructure::WoodlandMansion ? 80 : 32;
    const int originRegionX = floorDivide(originChunkX, spacing);
    const int originRegionZ = floorDivide(originChunkZ, spacing);
    for (int radius = 0; radius <= maximumRegionRadius; ++radius)
    {
        for (int dx = -radius; dx <= radius; ++dx)
        {
            for (int dz = -radius; dz <= radius; ++dz)
            {
                if (radius != 0 && std::abs(dx) != radius &&
                    std::abs(dz) != radius)
                    continue;
                int chunkX = originRegionX + dx;
                int chunkZ = originRegionZ + dz;
                bool candidate = false;
                if (structure == WorldStructure::Village)
                {
                    std::tie(chunkX, chunkZ) = scatteredCandidate(
                        worldSeed_, chunkX, chunkZ, 32, 8, 10387312LL);
                    candidate = villageBiome(
                        climateSampler(chunkX * 16 + 8, chunkZ * 16 + 8).biome);
                }
                else if (structure == WorldStructure::Temple)
                {
                    std::tie(chunkX, chunkZ) = scatteredCandidate(
                        worldSeed_, chunkX, chunkZ, 32, 8, 14357617LL);
                    candidate = templeBiome(
                        climateSampler(chunkX * 16 + 8, chunkZ * 16 + 8).biome);
                }
                else if (structure == WorldStructure::OceanMonument)
                {
                    std::tie(chunkX, chunkZ) = triangularCandidate(
                        worldSeed_, chunkX, chunkZ, 32, 5, 10387313LL);
                    candidate = monumentBiome(
                        climateSampler(chunkX * 16 + 8, chunkZ * 16 + 8).biome);
                }
                else if (structure == WorldStructure::WoodlandMansion)
                {
                    std::tie(chunkX, chunkZ) = triangularCandidate(
                        worldSeed_, chunkX, chunkZ, 80, 20, 10387319LL);
                    candidate = mansionBiome(
                        climateSampler(chunkX * 16 + 8, chunkZ * 16 + 8).biome);
                }
                else
                {
                    candidate = isMineshaftChunk(chunkX, chunkZ);
                }
                if (!candidate)
                    continue;
                const int x = chunkX * 16 + 8;
                const int z = chunkZ * 16 + 8;
                const double distance = std::hypot(
                    static_cast<double>(x - blockX),
                    static_cast<double>(z - blockZ));
                if (distance < closestDistance)
                {
                    closestDistance = distance;
                    closest = StructureLocation{
                        structure, x, z, climateSampler(x, z).biome};
                }
            }
        }
        if (closest && radius > 1)
            break;
    }
    return closest;
}
