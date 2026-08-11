#include "worldgen/PopulationGenerator.h"

#include "Block.h"
#include "Chunk.h"
#include "content/BlockState.h"
#include "content/ContentCatalog.h"
#include "core/ResourceLocation.h"
#include "worldgen/BetaSimplexOctaves.h"
#include "worldgen/Biome.h"
#include "worldgen/BiomeMap.h"
#include "worldgen/ClayGenerator.h"
#include "worldgen/DecorationGenerator.h"
#include "worldgen/DungeonGenerator.h"
#include "worldgen/FossilGenerator.h"
#include "worldgen/JavaRandom.h"
#include "worldgen/LakeGenerator.h"
#include "worldgen/MinableGenerator.h"
#include "worldgen/OreGenerator.h"
#include "worldgen/StructureGenerator.h"
#include "worldgen/TreeGenerator.h"
#include "worldgen/Vanilla112State.h"
#include "worldgen/WorldGenerationContext.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <optional>
#include <string_view>
#include <utility>

namespace
{
std::int64_t makeOdd(std::int64_t value) noexcept
{
    return value / 2LL * 2LL + 1LL;
}

std::int64_t addWrap(std::int64_t first, std::int64_t second) noexcept
{
    return std::bit_cast<std::int64_t>(
        static_cast<std::uint64_t>(first) +
        static_cast<std::uint64_t>(second));
}

std::int64_t mulWrap(std::int64_t first, std::int64_t second) noexcept
{
    return std::bit_cast<std::int64_t>(
        static_cast<std::uint64_t>(first) *
        static_cast<std::uint64_t>(second));
}

std::int64_t populationSeed(
    int chunkX,
    std::int64_t xMultiplier,
    int chunkZ,
    std::int64_t zMultiplier,
    std::int64_t worldSeed) noexcept
{
    const std::uint64_t value =
        static_cast<std::uint64_t>(mulWrap(chunkX, xMultiplier)) +
        static_cast<std::uint64_t>(mulWrap(chunkZ, zMultiplier));
    return std::bit_cast<std::int64_t>(
        value ^ static_cast<std::uint64_t>(worldSeed));
}

int floorDivide(int value, int divisor) noexcept
{
    int quotient = value / divisor;
    const int remainder = value % divisor;
    if (remainder != 0 && ((remainder < 0) != (divisor < 0)))
        --quotient;
    return quotient;
}

bool isVillageBiome(BiomeId biome) noexcept
{
    return biome == VanillaBiomes::Plains ||
           biome == VanillaBiomes::Desert ||
           biome == VanillaBiomes::Savanna ||
           biome == VanillaBiomes::Taiga;
}

bool villageStarts(std::int64_t worldSeed, int chunkX, int chunkZ)
{
    const int regionX = floorDivide(chunkX, 32);
    const int regionZ = floorDivide(chunkZ, 32);

    std::int64_t seed = mulWrap(regionX, 341873128712LL);
    seed = addWrap(seed, mulWrap(regionZ, 132897987541LL));
    seed = addWrap(seed, worldSeed);
    seed = addWrap(seed, 10387312LL);
    JavaRandom random(seed);

    const int candidateX = regionX * 32 + random.nextInt(24);
    const int candidateZ = regionZ * 32 + random.nextInt(24);
    if (candidateX != chunkX || candidateZ != chunkZ)
        return false;

    BiomeMap map(worldSeed);
    const auto sample = map.sampleGenerationArea(
        chunkX * 4 + 2,
        chunkZ * 4 + 2,
        1,
        1);
    return !sample.empty() && isVillageBiome(sample.front().biome);
}

const BetaSimplexOctaves& biomeGrassNoise()
{
    static JavaRandom random(2345LL);
    static BetaSimplexOctaves noise(random, 1);
    return noise;
}

bool isTaigaClass(BiomeId biome) noexcept
{
    return biome == VanillaBiomes::Taiga ||
           biome == VanillaBiomes::TaigaHills ||
           biome == VanillaBiomes::TaigaMountains ||
           biome == VanillaBiomes::ColdTaiga ||
           biome == VanillaBiomes::ColdTaigaHills ||
           biome == VanillaBiomes::ColdTaigaMountains ||
           biome == VanillaBiomes::MegaTaiga ||
           biome == VanillaBiomes::MegaTaigaHills ||
           biome == VanillaBiomes::MegaSpruceTaiga ||
           biome == VanillaBiomes::MegaSpruceTaigaHills;
}

bool isMegaTaiga(BiomeId biome) noexcept
{
    return biome == VanillaBiomes::MegaTaiga ||
           biome == VanillaBiomes::MegaTaigaHills ||
           biome == VanillaBiomes::MegaSpruceTaiga ||
           biome == VanillaBiomes::MegaSpruceTaigaHills;
}

bool isForestClass(BiomeId biome) noexcept
{
    return biome == VanillaBiomes::Forest ||
           biome == VanillaBiomes::ForestHills ||
           biome == VanillaBiomes::FlowerForest ||
           biome == VanillaBiomes::BirchForest ||
           biome == VanillaBiomes::BirchForestHills ||
           biome == VanillaBiomes::BirchForestMountains ||
           biome == VanillaBiomes::BirchForestHillsMountains ||
           biome == VanillaBiomes::RoofedForest ||
           biome == VanillaBiomes::RoofedForestMountains;
}

bool isRoofedForest(BiomeId biome) noexcept
{
    return biome == VanillaBiomes::RoofedForest ||
           biome == VanillaBiomes::RoofedForestMountains;
}

bool isPlainsClass(BiomeId biome) noexcept
{
    return biome == VanillaBiomes::Plains ||
           biome == VanillaBiomes::SunflowerPlains;
}

bool isJungleClass(BiomeId biome) noexcept
{
    return biome == VanillaBiomes::Jungle ||
           biome == VanillaBiomes::JungleHills ||
           biome == VanillaBiomes::JungleEdge ||
           biome == VanillaBiomes::JungleMountains ||
           biome == VanillaBiomes::JungleEdgeMountains;
}

bool isHillsClass(BiomeId biome) noexcept
{
    return biome == VanillaBiomes::ExtremeHills ||
           biome == VanillaBiomes::ExtremeHillsEdge ||
           biome == VanillaBiomes::ExtremeHillsPlus ||
           biome == VanillaBiomes::ExtremeHillsMountains ||
           biome == VanillaBiomes::ExtremeHillsPlusMountains;
}

bool isMesa(BiomeId biome) noexcept
{
    return biome == VanillaBiomes::Mesa ||
           biome == VanillaBiomes::MesaPlateauF ||
           biome == VanillaBiomes::MesaPlateau ||
           biome == VanillaBiomes::MesaBryce ||
           biome == VanillaBiomes::MesaPlateauFMountains ||
           biome == VanillaBiomes::MesaPlateauMountains;
}

mc::content::BlockState namedState(
    std::string_view name,
    BlockType fallback)
{
    const auto* catalog = mc::content::ContentCatalog::active();
    if (catalog != nullptr)
    {
        if (const auto state = catalog->state(
                mc::core::ResourceLocation("minecraft", name)))
            return *state;
    }
    return mc::content::BlockState(fallback);
}

bool sameRuntimeBlock(
    mc::content::BlockState first,
    mc::content::BlockState second) noexcept
{
    return first.blockRuntimeId() == second.blockRuntimeId();
}

bool isVanillaStoneBlock(mc::content::BlockState state) noexcept
{
    if (state.block() == BlockType::Stone) return true;
    const std::string_view p = mc112::path(state);
    return p == "stone" || p == "granite" || p == "polished_granite" ||
           p == "diorite" || p == "polished_diorite" ||
           p == "andesite" || p == "polished_andesite";
}

bool isVanillaDirtBlock(mc::content::BlockState state) noexcept
{
    if (state.block() == BlockType::Dirt) return true;
    const std::string_view p = mc112::path(state);
    return p == "dirt" || p == "coarse_dirt" || p == "podzol";
}

void generateSpring(
    WorldGenerationContext& context,
    BlockType liquid,
    int x,
    int y,
    int z)
{
    // WorldGenLiquids compares the Block object to Blocks.STONE. In 1.12.2
    // every stone metadata variant is the same block, so the split resource
    // catalog must treat all seven variants as that block here.
    if (!isVanillaStoneBlock(context.getBlockState(x, y + 1, z)) ||
        !isVanillaStoneBlock(context.getBlockState(x, y - 1, z)))
        return;

    const auto current = context.getBlockState(x, y, z);
    if (!mc112::isAir(current) && !isVanillaStoneBlock(current))
        return;

    int stone = 0;
    int air = 0;
    for (const auto [dx, dz] : {
             std::pair{-1, 0}, std::pair{1, 0},
             std::pair{0, -1}, std::pair{0, 1}})
    {
        const auto block = context.getBlockState(x + dx, y, z + dz);
        if (isVanillaStoneBlock(block)) ++stone;
        if (mc112::isAir(block)) ++air;
    }
    if (stone == 3 && air == 1)
        context.setBlock(x, y, z, liquid);
}

bool generateDoublePlant(
    WorldGenerationContext& context,
    JavaRandom& random,
    std::string_view variant,
    int x,
    int y,
    int z)
{
    // WorldGenDoublePlant::generate: 64 scatter attempts. BlockDoublePlant
    // inherits BlockBush, so the lower half requires grass/dirt/farmland and
    // the upper position must be air. The dirt test intentionally includes
    // all 1.12 dirt variants because vanilla checks the block, not metadata.
bool generated = false;

const std::array<mc112::Property, 2> lowerProperties = {
    mc112::Property{"variant", std::string(variant)},
    mc112::Property{"half", "lower"}
};

const std::array<mc112::Property, 2> upperProperties = {
    mc112::Property{"variant", std::string(variant)},
    mc112::Property{"half", "upper"}
};

const auto lowerState = mc112::vanilla112State(
    "minecraft:double_plant", lowerProperties);

const auto upperState = mc112::vanilla112State(
    "minecraft:double_plant", upperProperties);

    for (int attempt = 0; attempt < 64; ++attempt)
    {
        const int px = x + random.nextInt(8) - random.nextInt(8);
        const int py = y + random.nextInt(4) - random.nextInt(4);
        const int pz = z + random.nextInt(8) - random.nextInt(8);
        if (py <= 0 || py >= Chunk::HEIGHT - 2)
            continue;
        if (!mc112::isAir(context.getBlockState(px, py, pz)) ||
            !mc112::isAir(context.getBlockState(px, py + 1, pz)))
            continue;

        const auto below = context.getBlockState(px, py - 1, pz);
        const std::string_view soil = mc112::path(below);
        const bool canSustain =
            below.block() == BlockType::Grass ||
            below.block() == BlockType::Dirt ||
            below.block() == BlockType::Farmland ||
            soil == "grass" || soil == "grass_block" ||
            soil == "dirt" || soil == "coarse_dirt" || soil == "podzol" ||
            soil == "farmland";
        if (!canSustain)
            continue;

        context.setBlockState(px, py, pz, lowerState);
        context.setBlockState(px, py + 1, pz, upperState);
        generated = true;
    }
    return generated;
}

void generateForestRock(
    WorldGenerationContext& context,
    JavaRandom& random,
    int x,
    int y,
    int z)
{
    // WorldGenBlockBlob(MOSSY_COBBLESTONE, 0).
    while (y > 3)
    {
        const BlockType below = context.getBlock(x, y - 1, z);
        if (below == BlockType::Grass ||
            below == BlockType::Dirt ||
            below == BlockType::Podzol ||
            below == BlockType::Stone)
            break;
        --y;
    }
    if (y <= 3)
        return;

    constexpr int startRadius = 0;
    for (int pass = 0; pass < 3; ++pass)
    {
        const int radiusX = startRadius + random.nextInt(2);
        const int radiusY = startRadius + random.nextInt(2);
        const int radiusZ = startRadius + random.nextInt(2);
        const float radius =
            static_cast<float>(radiusX + radiusY + radiusZ) * 0.333F + 0.5F;
        const float radiusSquared = radius * radius;

        for (int px = x - radiusX; px <= x + radiusX; ++px)
            for (int py = y - radiusY; py <= y + radiusY; ++py)
                for (int pz = z - radiusZ; pz <= z + radiusZ; ++pz)
                {
                    const int dx = px - x;
                    const int dy = py - y;
                    const int dz = pz - z;
                    if (static_cast<float>(dx * dx + dy * dy + dz * dz) <=
                        radiusSquared)
                        context.setBlock(px, py, pz, BlockType::MossyCobblestone);
                }

        const int shiftX = random.nextInt(2);
        const int shiftY = random.nextInt(2);
        const int shiftZ = random.nextInt(2);
        x += -(startRadius + 1) + shiftX;
        y -= shiftY;
        z += -(startRadius + 1) + shiftZ;
    }
}

void generateIcePath(
    WorldGenerationContext& context,
    JavaRandom& random,
    int x,
    int y,
    int z)
{
    const mc::content::BlockState packedIce =
        namedState("packed_ice", BlockType::Ice);
    const mc::content::BlockState snow =
        namedState("snow", BlockType::Snow);

    y = std::max(3, y - 1);
    const int radius = random.nextInt(2) + 2; // basePathWidth = 4
    for (int px = x - radius; px <= x + radius; ++px)
    {
        for (int pz = z - radius; pz <= z + radius; ++pz)
        {
            const int dx = px - x;
            const int dz = pz - z;
            if (dx * dx + dz * dz > radius * radius)
                continue;

            for (int py = y - 1; py <= y + 1; ++py)
            {
                const auto state = context.getBlockState(px, py, pz);
                const BlockType block = state.block();
                if (block == BlockType::Dirt ||
                    block == BlockType::Snow ||
                    block == BlockType::Ice ||
                    sameRuntimeBlock(state, snow))
                    context.setBlockState(px, py, pz, packedIce);
            }
        }
    }
}

void generateIceSpike(
    WorldGenerationContext& context,
    JavaRandom& random,
    int x,
    int y,
    int z)
{
    const mc::content::BlockState packedIce =
        namedState("packed_ice", BlockType::Ice);
    const mc::content::BlockState snow =
        namedState("snow", BlockType::Snow);

    // world.getHeight() gives the first air block; the vanilla generator then
    // descends through air to the snow surface without consuming RNG.
    y = std::max(3, y - 1);
    y += random.nextInt(4);
    const int height = random.nextInt(4) + 7;
    const int baseRadius = height / 4 + random.nextInt(2);
    if (baseRadius > 1 && random.nextInt(60) == 0)
        y += 10 + random.nextInt(30);

    for (int layer = 0; layer < height; ++layer)
    {
        const float radius =
            (1.0F - static_cast<float>(layer) / static_cast<float>(height)) *
            static_cast<float>(baseRadius);
        const int ceilRadius = static_cast<int>(std::ceil(radius));
        for (int dx = -ceilRadius; dx <= ceilRadius; ++dx)
        {
            const float fx = static_cast<float>(std::abs(dx)) - 0.25F;
            for (int dz = -ceilRadius; dz <= ceilRadius; ++dz)
            {
                const float fz = static_cast<float>(std::abs(dz)) - 0.25F;
                const bool inside =
                    (dx == 0 && dz == 0) ||
                    fx * fx + fz * fz <= radius * radius;
                if (!inside)
                    continue;

                const bool onSquareEdge =
                    dx == -ceilRadius || dx == ceilRadius ||
                    dz == -ceilRadius || dz == ceilRadius;
                if (onSquareEdge && random.nextFloat() > 0.75F)
                    continue;

                const auto topState = context.getBlockState(x + dx, y + layer, z + dz);
                const BlockType topBlock = topState.block();
                if (topState.isAir() || topBlock == BlockType::Dirt ||
                    topBlock == BlockType::Snow || topBlock == BlockType::Ice ||
                    sameRuntimeBlock(topState, snow))
                    context.setBlockState(x + dx, y + layer, z + dz, packedIce);

                if (layer != 0 && ceilRadius > 1)
                {
                    const auto bottomState =
                        context.getBlockState(x + dx, y - layer, z + dz);
                    const BlockType bottomBlock = bottomState.block();
                    if (bottomState.isAir() || bottomBlock == BlockType::Dirt ||
                        bottomBlock == BlockType::Snow || bottomBlock == BlockType::Ice ||
                        sameRuntimeBlock(bottomState, snow))
                        context.setBlockState(x + dx, y - layer, z + dz, packedIce);
                }
            }
        }
    }

    const int rootRadius = std::clamp(baseRadius - 1, 0, 1);
    for (int dx = -rootRadius; dx <= rootRadius; ++dx)
    {
        for (int dz = -rootRadius; dz <= rootRadius; ++dz)
        {
            int py = y - 1;
            int remaining = 50;
            if (std::abs(dx) == 1 && std::abs(dz) == 1)
                remaining = random.nextInt(5);

            while (py > 50)
            {
                const auto state = context.getBlockState(x + dx, py, z + dz);
                const BlockType block = state.block();
                const bool replaceable =
                    state.isAir() || block == BlockType::Dirt ||
                    block == BlockType::Snow || block == BlockType::Ice ||
                    sameRuntimeBlock(state, snow) ||
                    sameRuntimeBlock(state, packedIce);
                if (!replaceable)
                    break;

                context.setBlockState(x + dx, py, z + dz, packedIce);
                --py;
                --remaining;
                if (remaining <= 0)
                {
                    py -= random.nextInt(5) + 1;
                    remaining = random.nextInt(5);
                }
            }
        }
    }
}

bool generateSandPatch(
    WorldGenerationContext& context,
    JavaRandom& random,
    BlockType replacement,
    int x,
    int y,
    int z,
    int configuredRadius)
{
    // WorldGenSand: water-only start, radius random(radius-2)+2, replace
    // dirt/grass in a five-block-thick disk.
    if (!mc112::isWater(context.getBlockState(x, y, z)))
        return false;

    const int radius = random.nextInt(configuredRadius - 2) + 2;
    for (int px = x - radius; px <= x + radius; ++px)
    {
        for (int pz = z - radius; pz <= z + radius; ++pz)
        {
            const int dx = px - x;
            const int dz = pz - z;
            if (dx * dx + dz * dz > radius * radius)
                continue;

            for (int py = y - 2; py <= y + 2; ++py)
            {
                const auto current = context.getBlockState(px, py, pz);
                if (isVanillaDirtBlock(current) ||
                    current.block() == BlockType::Grass ||
                    mc112::path(current) == "grass" ||
                    mc112::path(current) == "grass_block")
                    context.setBlock(px, py, pz, replacement);
            }
        }
    }
    return true;
}

mc::content::BlockState flowerState(std::string_view type)
{
    if (type == "houstonia") return mc112::state("azure_bluet");
    return mc112::state(type);
}

bool canSustainVanillaBush(mc::content::BlockState below) noexcept
{
    const std::string_view soil = mc112::path(below);
    return below.block() == BlockType::Grass ||
           below.block() == BlockType::Dirt ||
           below.block() == BlockType::Farmland ||
           soil == "grass" || soil == "grass_block" ||
           soil == "dirt" || soil == "coarse_dirt" ||
           soil == "podzol" || soil == "farmland";
}

void generateFlowerPatch(
    WorldGenerationContext& context,
    JavaRandom& random,
    mc::content::BlockState flower,
    int x,
    int y,
    int z)
{
    // WorldGenFlowers::generate. Overworld has a sky, so only the vanilla
    // y<255 limit applies in addition to BlockBush::canBlockStay.
    for (int attempt = 0; attempt < 64; ++attempt)
    {
        const int px = x + random.nextInt(8) - random.nextInt(8);
        const int py = y + random.nextInt(4) - random.nextInt(4);
        const int pz = z + random.nextInt(8) - random.nextInt(8);
        if (py <= 0 || py >= 255)
            continue;
        if (mc112::isAir(context.getBlockState(px, py, pz)) &&
            canSustainVanillaBush(context.getBlockState(px, py - 1, pz)))
            context.setBlockState(px, py, pz, flower);
    }
}

mc::content::BlockState pickFlowerForBiome(
    JavaRandom& random,
    BiomeId biome,
    int x,
    int z)
{
    // BiomeSwamp::pickRandomFlower.
    if (biome == VanillaBiomes::Swampland ||
        biome == VanillaBiomes::SwamplandMountains)
        return flowerState("blue_orchid");

    // BiomeForest(FLOWER)::pickRandomFlower. EnumFlowerType.values() order is
    // DANDELION, POPPY, BLUE_ORCHID, ALLIUM, HOUSTONIA, four tulips, OXEYE.
    // BLUE_ORCHID is replaced with POPPY in flower forests.
    if (biome == VanillaBiomes::FlowerForest)
    {
        static constexpr std::array<std::string_view, 10> flowers{{
            "dandelion", "poppy", "blue_orchid", "allium", "houstonia",
            "red_tulip", "orange_tulip", "white_tulip", "pink_tulip",
            "oxeye_daisy"}};
        const double normalized = std::clamp(
            (1.0 + biomeGrassNoise().value(
                static_cast<double>(x) / 48.0,
                static_cast<double>(z) / 48.0)) / 2.0,
            0.0,
            0.9999);
        std::string_view chosen = flowers[static_cast<std::size_t>(
            normalized * static_cast<double>(flowers.size()))];
        if (chosen == "blue_orchid") chosen = "poppy";
        return flowerState(chosen);
    }

    // BiomePlains::pickRandomFlower, including exact Random calls/order.
    if (isPlainsClass(biome))
    {
        const double noise = biomeGrassNoise().value(
            static_cast<double>(x) / 200.0,
            static_cast<double>(z) / 200.0);
        if (noise < -0.8)
        {
            static constexpr std::array<std::string_view, 4> tulips{{
                "orange_tulip", "red_tulip", "pink_tulip", "white_tulip"}};
            return flowerState(tulips[static_cast<std::size_t>(random.nextInt(4))]);
        }
        if (random.nextInt(3) > 0)
        {
            static constexpr std::array<std::string_view, 3> redFlowers{{
                "poppy", "houstonia", "oxeye_daisy"}};
            return flowerState(redFlowers[static_cast<std::size_t>(random.nextInt(3))]);
        }
        return flowerState("dandelion");
    }

    // Biome::pickRandomFlower.
    return random.nextInt(3) > 0
        ? flowerState("dandelion")
        : flowerState("poppy");
}

BlockType grassForBiome(JavaRandom& random, BiomeId biome)
{
    if (isTaigaClass(biome))
        return random.nextInt(5) > 0 ? BlockType::Fern : BlockType::TallGrass;
    if (isJungleClass(biome))
        return random.nextInt(4) == 0 ? BlockType::Fern : BlockType::TallGrass;
    return BlockType::TallGrass;
}
}

PopulationGenerator::PopulationGenerator(std::int64_t worldSeed)
    : worldSeed_(worldSeed)
{
}

void PopulationGenerator::populate(
    Chunk& targetChunk,
    WorldGenerationContext& context,
    const StructureGenerator& structures) const
{
    JavaRandom seedRandom(worldSeed_);
    const std::int64_t xMultiplier = makeOdd(seedRandom.nextLong());
    const std::int64_t zMultiplier = makeOdd(seedRandom.nextLong());

    const LakeGenerator waterLake(BlockType::Water);
    const LakeGenerator lavaLake(BlockType::Lava);
    const DungeonGenerator dungeons;
    const mc112::FossilGenerator fossils(worldSeed_);
    const OreGenerator ores;
    const ClayGenerator clay(4);
    const TreeGenerator trees;
    const DecorationGenerator decorations;
    const MinableGenerator mesaGold(BlockType::GoldOre, 9);
    const MinableGenerator silverfish(
        mc112::state("stone_monster_egg"), 9);

    const auto generateTree = [&context, &trees](
        JavaRandom& random,
        BiomeId biome,
        int x,
        int y,
        int z)
    {
        context.beginIsolatedFeature();
        const bool generated = trees.generateForBiome(
            context, random, biome, x, y, z);
        context.finishIsolatedFeature(generated);
        return generated;
    };

    // BiomeDecorator positions are offset +8 from the population chunk, so a
    // target chunk can receive decoration from itself and its -X/-Z neighbors.
    for (int sourceChunkX = targetChunk.getChunkX() - 1;
         sourceChunkX <= targetChunk.getChunkX(); ++sourceChunkX)
    {
        for (int sourceChunkZ = targetChunk.getChunkZ() - 1;
             sourceChunkZ <= targetChunk.getChunkZ(); ++sourceChunkZ)
        {
            JavaRandom random(populationSeed(
                sourceChunkX,
                xMultiplier,
                sourceChunkZ,
                zMultiplier,
                worldSeed_));

            const int originX = sourceChunkX * 16;
            const int originZ = sourceChunkZ * 16;
            const ClimateSample climate =
                context.sampleClimate(originX + 16, originZ + 16);
            const BiomeDefinition* biome =
                BiomeRegistry::active().find(climate.biome);
            // ChunkGeneratorOverworld::populate runs all MapGenStructure
            // post-processing first with this exact same Random.
            const PopulationStructureResult structureResult =
                structures.populateSource(
                    context, random, sourceChunkX, sourceChunkZ);
            const bool village = structureResult.villageGenerated;

            // ChunkGeneratorOverworld::populate after structure postprocess.
            if (climate.biome != VanillaBiomes::Desert &&
                climate.biome != VanillaBiomes::DesertHills &&
                !village && random.nextInt(4) == 0)
            {
                const int x = originX + random.nextInt(16) + 8;
                const int y = random.nextInt(256);
                const int z = originZ + random.nextInt(16) + 8;
                waterLake.generate(context, random, x, y, z);
            }

            if (!village && random.nextInt(8) == 0)
            {
                const int x = originX + random.nextInt(16) + 8;
                const int nestedHeight = random.nextInt(248) + 8;
                const int y = random.nextInt(nestedHeight);
                const int z = originZ + random.nextInt(16) + 8;
                if (y < 63 || random.nextInt(10) == 0)
                    lavaLake.generate(context, random, x, y, z);
            }

            for (int attempt = 0; attempt < 8; ++attempt)
            {
                const int x = originX + random.nextInt(16) + 8;
                const int y = random.nextInt(256);
                const int z = originZ + random.nextInt(16) + 8;
                dungeons.generate(context, random, x, y, z);
            }

            int flowerCount = biome ? biome->flowersPerChunk : 2;
            int grassCount = biome ? biome->grassPerChunk : 1;

            // Biome-specific decorate hooks that run BEFORE BiomeDecorator.
            if (isPlainsClass(climate.biome))
            {
                const double plainsNoise = biomeGrassNoise().value(
                    static_cast<double>(originX + 8) / 200.0,
                    static_cast<double>(originZ + 8) / 200.0);
                if (plainsNoise < -0.8)
                {
                    flowerCount = 15;
                    grassCount = 5;
                }
                else
                {
                    flowerCount = 4;
                    grassCount = 10;
                    for (int i = 0; i < 7; ++i)
                    {
                        const int x = originX + random.nextInt(16) + 8;
                        const int z = originZ + random.nextInt(16) + 8;
                        const int bound = context.getHeightValue(x, z) + 32;
                        if (bound > 0)
                        {
                            const int y = random.nextInt(bound);
                            (void)generateDoublePlant(
                                context, random, "double_grass", x, y, z);
                        }
                    }
                }
                if (climate.biome == VanillaBiomes::SunflowerPlains)
                {
                    for (int i = 0; i < 10; ++i)
                    {
                        const int x = originX + random.nextInt(16) + 8;
                        const int z = originZ + random.nextInt(16) + 8;
                        const int bound = context.getHeightValue(x, z) + 32;
                        if (bound > 0)
                        {
                            const int y = random.nextInt(bound);
                            (void)generateDoublePlant(
                                context, random, "sunflower", x, y, z);
                        }
                    }
                }
            }

            if (isForestClass(climate.biome))
            {
                if (isRoofedForest(climate.biome))
                {
                    for (int gridX = 0; gridX < 4; ++gridX)
                    {
                        for (int gridZ = 0; gridZ < 4; ++gridZ)
                        {
                            const int x = originX + gridX * 4 + 9 +
                                random.nextInt(3);
                            const int z = originZ + gridZ * 4 + 9 +
                                random.nextInt(3);
                            const int y = context.getHeightValue(x, z);
                            if (random.nextInt(20) == 0)
                            {
                                context.beginIsolatedFeature();
                                const bool generated =
                                    decorations.generateBigMushroom(
                                        context, random, x, y, z);
                                context.finishIsolatedFeature(generated);
                            }
                            else
                            {
                                (void)generateTree(
                                    random, climate.biome, x, y, z);
                            }
                        }
                    }
                }

                int doublePlantCount = random.nextInt(5) - 3;
                if (climate.biome == VanillaBiomes::FlowerForest)
                    doublePlantCount += 2;
                for (int i = 0; i < doublePlantCount; ++i)
                {
                    static constexpr std::array<std::string_view, 3> variants{{
                        "syringa", "double_rose", "paeonia"}};
                    const std::string_view variant = variants[
                        static_cast<std::size_t>(random.nextInt(3))];
                    for (int retry = 0; retry < 5; ++retry)
                    {
                        const int x = originX + random.nextInt(16) + 8;
                        const int z = originZ + random.nextInt(16) + 8;
                        const int bound = context.getHeightValue(x, z) + 32;
                        if (bound <= 0)
                            continue;
                        const int y = random.nextInt(bound);
                        if (generateDoublePlant(
                                context, random, variant, x, y, z))
                            break;
                    }
                }
            }

            if (climate.biome == VanillaBiomes::Savanna ||
                climate.biome == VanillaBiomes::SavannaPlateau)
            {
                for (int i = 0; i < 7; ++i)
                {
                    const int x = originX + random.nextInt(16) + 8;
                    const int z = originZ + random.nextInt(16) + 8;
                    const int bound = context.getHeightValue(x, z) + 32;
                    if (bound > 0)
                    {
                        const int y = random.nextInt(bound);
                        (void)generateDoublePlant(
                            context, random, "double_grass", x, y, z);
                    }
                }
            }

            if (isTaigaClass(climate.biome))
            {
                if (isMegaTaiga(climate.biome))
                {
                    const int rockCount = random.nextInt(3);
                    for (int i = 0; i < rockCount; ++i)
                    {
                        const int x = originX + random.nextInt(16) + 8;
                        const int z = originZ + random.nextInt(16) + 8;
                        generateForestRock(
                            context,
                            random,
                            x,
                            context.getHeightValue(x, z),
                            z);
                    }
                }

                for (int i = 0; i < 7; ++i)
                {
                    const int x = originX + random.nextInt(16) + 8;
                    const int z = originZ + random.nextInt(16) + 8;
                    const int bound = context.getHeightValue(x, z) + 32;
                    if (bound > 0)
                    {
                        const int y = random.nextInt(bound);
                        (void)generateDoublePlant(
                            context, random, "double_fern", x, y, z);
                    }
                }
            }

            if (climate.biome == VanillaBiomes::IcePlainsSpikes)
            {
                for (int i = 0; i < 3; ++i)
                {
                    const int x = originX + random.nextInt(16) + 8;
                    const int z = originZ + random.nextInt(16) + 8;
                    generateIceSpike(
                        context,
                        random,
                        x,
                        context.getHeightValue(x, z),
                        z);
                }
                for (int i = 0; i < 2; ++i)
                {
                    const int x = originX + random.nextInt(16) + 8;
                    const int z = originZ + random.nextInt(16) + 8;
                    generateIcePath(
                        context,
                        random,
                        x,
                        context.getHeightValue(x, z),
                        z);
                }
            }

            // BiomeDecorator::generateOres, including Mesa.Decorator's extra
            // gold immediately after the standard ore pass.
            ores.generate(context, random, originX, originZ);
            if (isMesa(climate.biome))
            {
                for (int i = 0; i < 20; ++i)
                {
                    const int x = originX + random.nextInt(16);
                    const int y = 32 + random.nextInt(48);
                    const int z = originZ + random.nextInt(16);
                    mesaGold.generate(context, random, x, y, z);
                }
            }

            const int sandPatches = biome ? biome->sandPatchesPerChunk : 3;
            for (int i = 0; i < sandPatches; ++i)
            {
                const int x = originX + random.nextInt(16) + 8;
                const int z = originZ + random.nextInt(16) + 8;
                generateSandPatch(
                    context,
                    random,
                    BlockType::Sand,
                    x,
                    context.getHeightValue(x, z) - 1,
                    z,
                    7);
            }

            const int clayPatches = biome ? biome->clayPatchesPerChunk : 1;
            for (int i = 0; i < clayPatches; ++i)
            {
                const int x = originX + random.nextInt(16) + 8;
                const int z = originZ + random.nextInt(16) + 8;
                clay.generate(
                    context,
                    random,
                    x,
                    context.getHeightValue(x, z) - 1,
                    z);
            }

            const int gravelPatches = biome ? biome->gravelPatchesPerChunk : 1;
            for (int i = 0; i < gravelPatches; ++i)
            {
                const int x = originX + random.nextInt(16) + 8;
                const int z = originZ + random.nextInt(16) + 8;
                generateSandPatch(
                    context,
                    random,
                    BlockType::Gravel,
                    x,
                    context.getHeightValue(x, z) - 1,
                    z,
                    6);
            }

            int treeCount = biome ? biome->treesPerChunk : 0;
            if (random.nextFloat() < (biome ? biome->extraTreeChance : 0.1F))
                ++treeCount;
            for (int i = 0; i < treeCount; ++i)
            {
                const int x = originX + random.nextInt(16) + 8;
                const int z = originZ + random.nextInt(16) + 8;
                (void)generateTree(
                    random,
                    climate.biome,
                    x,
                    context.getHeightValue(x, z),
                    z);
            }

            const int bigMushrooms = biome ? biome->bigMushroomsPerChunk : 0;
            for (int i = 0; i < bigMushrooms; ++i)
            {
                const int x = originX + random.nextInt(16) + 8;
                const int z = originZ + random.nextInt(16) + 8;
                context.beginIsolatedFeature();
                const bool generated = decorations.generateBigMushroom(
                    context,
                    random,
                    x,
                    context.getHeightValue(x, z),
                    z);
                context.finishIsolatedFeature(generated);
            }

            for (int i = 0; i < flowerCount; ++i)
            {
                const int x = originX + random.nextInt(16) + 8;
                const int z = originZ + random.nextInt(16) + 8;
                const int bound = context.getHeightValue(x, z) + 32;
                if (bound <= 0)
                    continue;
                const int y = random.nextInt(bound);
                const mc::content::BlockState flower = pickFlowerForBiome(
                    random, climate.biome, x, z);
                generateFlowerPatch(context, random, flower, x, y, z);
            }

            for (int i = 0; i < grassCount; ++i)
            {
                const int x = originX + random.nextInt(16) + 8;
                const int z = originZ + random.nextInt(16) + 8;
                const int bound = context.getHeightValue(x, z) * 2;
                if (bound <= 0)
                    continue;
                const int y = random.nextInt(bound);
                const BlockType grass = grassForBiome(random, climate.biome);
                decorations.generateTallGrass(
                    context, random, grass, x, y, z);
            }

            const int deadBushes = biome ? biome->deadBushesPerChunk : 0;
            for (int i = 0; i < deadBushes; ++i)
            {
                const int x = originX + random.nextInt(16) + 8;
                const int z = originZ + random.nextInt(16) + 8;
                const int bound = context.getHeightValue(x, z) * 2;
                if (bound <= 0)
                    continue;
                const int y = random.nextInt(bound);
                decorations.generateFlowers(
                    context, random, BlockType::DeadBush, x, y, z);
            }

            const int waterLilies =
                (climate.biome == VanillaBiomes::Swampland ||
                 climate.biome == VanillaBiomes::SwamplandMountains)
                    ? 4 : 0;
            for (int i = 0; i < waterLilies; ++i)
            {
                const int x = originX + random.nextInt(16) + 8;
                const int z = originZ + random.nextInt(16) + 8;
                const int bound = context.getHeightValue(x, z) * 2;
                if (bound <= 0)
                    continue;
                int y = random.nextInt(bound);
                while (y > 0 && context.getBlock(x, y - 1, z) == BlockType::Air)
                    --y;

                // WorldGenWaterlily: ten scatter attempts.
                const auto lily = namedState("waterlily", BlockType::Air);
                for (int attempt = 0; attempt < 10; ++attempt)
                {
                    const int px = x + random.nextInt(8) - random.nextInt(8);
                    const int py = y + random.nextInt(4) - random.nextInt(4);
                    const int pz = z + random.nextInt(8) - random.nextInt(8);
                    if (!lily.isAir() && py > 0 &&
                        context.getBlockState(px, py, pz).isAir() &&
                        context.getBlock(px, py - 1, pz) == BlockType::Water)
                        context.setBlockState(px, py, pz, lily);
                }
            }

            const int mushrooms = biome ? biome->mushroomsPerChunk : 0;
            for (int i = 0; i < mushrooms; ++i)
            {
                if (random.nextInt(4) == 0)
                {
                    const int x = originX + random.nextInt(16) + 8;
                    const int z = originZ + random.nextInt(16) + 8;
                    decorations.generateFlowers(
                        context,
                        random,
                        BlockType::BrownMushroom,
                        x,
                        context.getHeightValue(x, z),
                        z);
                }
                if (random.nextInt(8) == 0)
                {
                    const int x = originX + random.nextInt(16) + 8;
                    const int z = originZ + random.nextInt(16) + 8;
                    const int bound = context.getHeightValue(x, z) * 2;
                    if (bound > 0)
                    {
                        const int y = random.nextInt(bound);
                        decorations.generateFlowers(
                            context,
                            random,
                            BlockType::RedMushroom,
                            x,
                            y,
                            z);
                    }
                }
            }

            if (random.nextInt(4) == 0)
            {
                const int x = originX + random.nextInt(16) + 8;
                const int z = originZ + random.nextInt(16) + 8;
                const int bound = context.getHeightValue(x, z) * 2;
                if (bound > 0)
                {
                    const int y = random.nextInt(bound);
                    decorations.generateFlowers(
                        context,
                        random,
                        BlockType::BrownMushroom,
                        x,
                        y,
                        z);
                }
            }

            if (random.nextInt(8) == 0)
            {
                const int x = originX + random.nextInt(16) + 8;
                const int z = originZ + random.nextInt(16) + 8;
                const int bound = context.getHeightValue(x, z) * 2;
                if (bound > 0)
                {
                    const int y = random.nextInt(bound);
                    decorations.generateFlowers(
                        context,
                        random,
                        BlockType::RedMushroom,
                        x,
                        y,
                        z);
                }
            }

            const int configuredReeds = biome ? biome->reedsPerChunk : 0;
            for (int i = 0; i < configuredReeds; ++i)
            {
                const int x = originX + random.nextInt(16) + 8;
                const int z = originZ + random.nextInt(16) + 8;
                const int bound = context.getHeightValue(x, z) * 2;
                if (bound > 0)
                {
                    const int y = random.nextInt(bound);
                    decorations.generateReeds(context, random, x, y, z);
                }
            }
            for (int i = 0; i < 10; ++i)
            {
                const int x = originX + random.nextInt(16) + 8;
                const int z = originZ + random.nextInt(16) + 8;
                const int bound = context.getHeightValue(x, z) * 2;
                if (bound > 0)
                {
                    const int y = random.nextInt(bound);
                    decorations.generateReeds(context, random, x, y, z);
                }
            }

            if (random.nextInt(32) == 0)
            {
                const int x = originX + random.nextInt(16) + 8;
                const int z = originZ + random.nextInt(16) + 8;
                const int bound = context.getHeightValue(x, z) * 2;
                if (bound > 0)
                {
                    const int y = random.nextInt(bound);
                    decorations.generatePumpkins(context, random, x, y, z);
                }
            }

            const int cacti = biome ? biome->cactiPerChunk : 0;
            for (int i = 0; i < cacti; ++i)
            {
                const int x = originX + random.nextInt(16) + 8;
                const int z = originZ + random.nextInt(16) + 8;
                const int bound = context.getHeightValue(x, z) * 2;
                if (bound > 0)
                {
                    const int y = random.nextInt(bound);
                    decorations.generateCactus(context, random, x, y, z);
                }
            }

            if (!biome || biome->generateFalls)
            {
                for (int i = 0; i < 50; ++i)
                {
                    const int x = originX + random.nextInt(16) + 8;
                    const int z = originZ + random.nextInt(16) + 8;
                    const int upper = random.nextInt(248) + 8;
                    const int y = random.nextInt(upper);
                    generateSpring(context, BlockType::Water, x, y, z);
                }
                for (int i = 0; i < 20; ++i)
                {
                    const int x = originX + random.nextInt(16) + 8;
                    const int z = originZ + random.nextInt(16) + 8;
                    const int first = random.nextInt(240) + 8;
                    const int second = random.nextInt(first) + 8;
                    const int y = random.nextInt(second);
                    generateSpring(context, BlockType::Lava, x, y, z);
                }
            }

            // Biome hooks that run AFTER BiomeDecorator.
            if (isJungleClass(climate.biome))
            {
                const int x = originX + random.nextInt(16) + 8;
                const int z = originZ + random.nextInt(16) + 8;
                const int bound = context.getHeightValue(x, z) * 2;
                if (bound > 0)
                {
                    const int y = random.nextInt(bound);
                    decorations.generateMelons(context, random, x, y, z);
                }
                for (int i = 0; i < 50; ++i)
                {
                    const int vineX = originX + random.nextInt(16) + 8;
                    const int vineZ = originZ + random.nextInt(16) + 8;
                    decorations.generateVines(
                        context, random, vineX, 128, vineZ);
                }
            }

            if (isHillsClass(climate.biome))
            {
                const int emeralds = 3 + random.nextInt(6);
                for (int i = 0; i < emeralds; ++i)
                {
                    const int x = originX + random.nextInt(16);
                    const int y = random.nextInt(28) + 4;
                    const int z = originZ + random.nextInt(16);
                    if (isVanillaStoneBlock(context.getBlockState(x, y, z)))
                        context.setBlockState(x, y, z, mc112::state("emerald_ore"));
                }
                for (int i = 0; i < 7; ++i)
                {
                    const int x = originX + random.nextInt(16);
                    const int y = random.nextInt(64);
                    const int z = originZ + random.nextInt(16);
                    silverfish.generate(
                        context, random, x, y, z);
                }
            }

            if ((climate.biome == VanillaBiomes::Swampland ||
                 climate.biome == VanillaBiomes::SwamplandMountains) &&
                random.nextInt(64) == 0)
            {
                // BiomeSwamp::decorate -> WorldGenFossils. WorldGenFossils
                // intentionally ignores this population Random after the gate
                // and creates Chunk::getRandomWithSeed(987234911L).
                fossils.generate(context, sourceChunkX, sourceChunkZ);
            }

            // WorldEntitySpawner consumes RNG between biome.decorate and the
            // final freeze/snow pass in vanilla. Mob worldgen spawning belongs
            // to the entity subsystem, not this terrain-only port.

            // ChunkGeneratorOverworld shifts blockpos by +8 before the 16x16
            // precipitation-height freeze/snow loop.
            decorations.freezeAndSnow(
                context,
                originX + 8,
                originZ + 8,
                16,
                16);
        }
    }
}
