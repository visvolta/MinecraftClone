#include "TerrainGenerator.h"

#include "Chunk.h"
#include "worldgen/WorldGenerationContext.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace
{
std::size_t densityIndex(int x, int z, int y, int sizeZ, int sizeY)
{
    return static_cast<std::size_t>((x * sizeZ + z) * sizeY + y);
}

std::size_t climateIndex(int x, int z, int depth)
{
    return static_cast<std::size_t>(x * depth + z);
}
}

TerrainGenerator::TerrainGenerator(int seed)
    : seed_(seed),
      generatorRandom_(seed),
      chunkRandom_(seed),
      minLimitNoise_(generatorRandom_, 16),
      maxLimitNoise_(generatorRandom_, 16),
      mainNoise_(generatorRandom_, 8),
      // 1.12.2 calls this NoiseGeneratorPerlin. It is the simplex-octave
      // generator, not NoiseGeneratorOctaves/NoiseGeneratorImproved.
      surfaceDepthNoise_(generatorRandom_, 4),
      scaleNoise_(generatorRandom_, 10),
      depthNoise_(generatorRandom_, 16),
      mobSpawnerNoise_(generatorRandom_, 8),
      biomeMap_(seed),
      surfaceBuilder_(seed),
      caveGenerator_(seed),
      ravineGenerator_(seed),
      structureGenerator_(seed),
      populationGenerator_(seed)
{
}

TerrainGenerator::~TerrainGenerator() = default;

void TerrainGenerator::generateChunk(Chunk& chunk) const
{
    const auto cached = terrainCache_.find(terrainCacheKey(
        chunk.getChunkX(), chunk.getChunkZ()));
    if (cached != terrainCache_.end())
        chunk = *cached->second;
    else
        generateTerrainOnly(chunk);

    cacheTerrainChunk(chunk);

    WorldGenerationContext context(
        chunk,
        [this](int worldX, int worldY, int worldZ)
        {
            const int chunkX = floorDivide(worldX, Chunk::WIDTH);
            const int chunkZ = floorDivide(worldZ, Chunk::DEPTH);
            return terrainChunkAt(chunkX, chunkZ).getBlock(
                positiveModulo(worldX, Chunk::WIDTH),
                worldY,
                positiveModulo(worldZ, Chunk::DEPTH));
        },
        [this](int worldX, int worldZ)
        {
            const int chunkX = floorDivide(worldX, Chunk::WIDTH);
            const int chunkZ = floorDivide(worldZ, Chunk::DEPTH);
            return terrainChunkAt(chunkX, chunkZ).getMotionBlockingHeight(
                positiveModulo(worldX, Chunk::WIDTH),
                positiveModulo(worldZ, Chunk::DEPTH));
        },
        [this](int worldX, int worldZ)
        {
            return biomeMap_.sample(worldX, worldZ);
        });

    structureGenerator_.populate(chunk, context);
    populationGenerator_.populate(chunk, context);
}

void TerrainGenerator::generateTerrainOnly(Chunk& chunk) const
{
    chunk.clear();

    const std::vector<ClimateSample> climate = biomeMap_.sampleArea(
        chunk.getWorldOriginX(), chunk.getWorldOriginZ(),
        Chunk::WIDTH, Chunk::DEPTH);
    const std::vector<ClimateSample> generationBiomes =
        biomeMap_.sampleGenerationArea(
            chunk.getChunkX() * 4 - 2,
            chunk.getChunkZ() * 4 - 2,
            10,
            10);

    for (int localX = 0; localX < Chunk::WIDTH; ++localX)
    {
        for (int localZ = 0; localZ < Chunk::DEPTH; ++localZ)
        {
            const ClimateSample& sample = climate[
                climateIndex(localX, localZ, Chunk::DEPTH)];
            chunk.setBiome(localX, localZ, sample.biome);
            chunk.setClimate(localX, localZ,
                static_cast<float>(sample.temperature),
                static_cast<float>(sample.humidity));
        }
    }

    generateBaseTerrain(chunk, generationBiomes);

    // ChunkGeneratorOverworld::replaceBiomeBlocks uses this seed, with no
    // world-seed xor, before it invokes each biome's terrain replacement.
    chunkRandom_.setSeed(makeChunkSeed(chunk.getChunkX(), chunk.getChunkZ()));
    replaceSurfaceBlocks(chunk, climate);

    // ChunkGeneratorOverworld carves these after biome surface replacement.
    caveGenerator_.generate(chunk);
    ravineGenerator_.generate(chunk);
}

std::optional<StructureLocation> TerrainGenerator::findNearestStructure(
    WorldStructure structure,
    int worldX,
    int worldZ,
    int maximumRegionRadius) const
{
    return structureGenerator_.findNearest(
        structure, worldX, worldZ, maximumRegionRadius,
        [this](int x, int z) { return biomeMap_.sample(x, z); });
}

const Chunk& TerrainGenerator::terrainChunkAt(int chunkX, int chunkZ) const
{
    const std::uint64_t key = terrainCacheKey(chunkX, chunkZ);
    const auto found = terrainCache_.find(key);
    if (found != terrainCache_.end())
        return *found->second;

    auto terrain = std::make_unique<Chunk>(chunkX, chunkZ);
    generateTerrainOnly(*terrain);
    const Chunk& result = *terrain;
    terrainCache_.emplace(key, std::move(terrain));
    terrainCacheOrder_.push_back(key);

    constexpr std::size_t maximumCachedTerrainChunks = 48;
    while (terrainCacheOrder_.size() > maximumCachedTerrainChunks)
    {
        const std::uint64_t oldest = terrainCacheOrder_.front();
        terrainCacheOrder_.pop_front();
        if (oldest != key)
            terrainCache_.erase(oldest);
    }
    return result;
}

void TerrainGenerator::cacheTerrainChunk(const Chunk& chunk) const
{
    const std::uint64_t key = terrainCacheKey(chunk.getChunkX(), chunk.getChunkZ());
    if (terrainCache_.contains(key))
        return;
    terrainCache_.emplace(key, std::make_unique<Chunk>(chunk));
    terrainCacheOrder_.push_back(key);
}

std::uint64_t TerrainGenerator::terrainCacheKey(int chunkX, int chunkZ) noexcept
{
    return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(chunkX)) << 32U) |
        static_cast<std::uint32_t>(chunkZ);
}

int TerrainGenerator::floorDivide(int value, int divisor) noexcept
{
    int quotient = value / divisor;
    const int remainder = value % divisor;
    if (remainder != 0 && ((remainder < 0) != (divisor < 0)))
        --quotient;
    return quotient;
}

int TerrainGenerator::positiveModulo(int value, int divisor) noexcept
{
    const int remainder = value % divisor;
    return remainder < 0 ? remainder + std::abs(divisor) : remainder;
}

int TerrainGenerator::getSeed() const noexcept { return seed_; }

int TerrainGenerator::getTerrainHeight(int, int) const
{
    return SurfaceBuilder::SEA_LEVEL;
}

void TerrainGenerator::generateBaseTerrain(
    Chunk& chunk,
    const std::vector<ClimateSample>& climate) const
{
    constexpr int horizontalCells = 4;
    constexpr int densitySizeX = 5;
    constexpr int densitySizeY = 33;
    constexpr int densitySizeZ = 5;

    initializeNoiseField(
        chunk.getChunkX() * horizontalCells,
        0,
        chunk.getChunkZ() * horizontalCells,
        densitySizeX,
        densitySizeY,
        densitySizeZ,
        climate);

    // This is the interpolation order used by setBlocksInChunk: 4x4x8
    // density cells expanded to the 16x16x256 ChunkPrimer.
    for (int cellX = 0; cellX < horizontalCells; ++cellX)
    {
        for (int cellZ = 0; cellZ < horizontalCells; ++cellZ)
        {
            for (int densityY = 0; densityY < 32; ++densityY)
            {
                double d00 = densityField_[densityIndex(
                    cellX, cellZ, densityY, densitySizeZ, densitySizeY)];
                double d01 = densityField_[densityIndex(
                    cellX, cellZ + 1, densityY, densitySizeZ, densitySizeY)];
                double d10 = densityField_[densityIndex(
                    cellX + 1, cellZ, densityY, densitySizeZ, densitySizeY)];
                double d11 = densityField_[densityIndex(
                    cellX + 1, cellZ + 1, densityY, densitySizeZ, densitySizeY)];

                const double dy00 = (densityField_[densityIndex(
                    cellX, cellZ, densityY + 1, densitySizeZ, densitySizeY)] - d00) * 0.125;
                const double dy01 = (densityField_[densityIndex(
                    cellX, cellZ + 1, densityY + 1, densitySizeZ, densitySizeY)] - d01) * 0.125;
                const double dy10 = (densityField_[densityIndex(
                    cellX + 1, cellZ, densityY + 1, densitySizeZ, densitySizeY)] - d10) * 0.125;
                const double dy11 = (densityField_[densityIndex(
                    cellX + 1, cellZ + 1, densityY + 1, densitySizeZ, densitySizeY)] - d11) * 0.125;

                for (int subY = 0; subY < 8; ++subY)
                {
                    double x0 = d00;
                    double x1 = d01;
                    const double dx0 = (d10 - d00) * 0.25;
                    const double dx1 = (d11 - d01) * 0.25;

                    for (int subX = 0; subX < 4; ++subX)
                    {
                        // Vanilla starts at d10-d16 and increments before the
                        // first Z block. Algebraically this equals x0 here.
                        double density = x0;
                        const double dz = (x1 - x0) * 0.25;
                        for (int subZ = 0; subZ < 4; ++subZ)
                        {
                            const int localX = cellX * 4 + subX;
                            const int localY = densityY * 8 + subY;
                            const int localZ = cellZ * 4 + subZ;
                            BlockType block = BlockType::Air;
                            if (density > 0.0)
                                block = BlockType::Stone;
                            else if (localY < SurfaceBuilder::SEA_LEVEL)
                                block = BlockType::Water;
                            chunk.setBlock(localX, localY, localZ, block);
                            density += dz;
                        }
                        x0 += dx0;
                        x1 += dx1;
                    }
                    d00 += dy00;
                    d01 += dy01;
                    d10 += dy10;
                    d11 += dy11;
                }
            }
        }
    }
}

void TerrainGenerator::replaceSurfaceBlocks(
    Chunk& chunk,
    const std::vector<ClimateSample>& climate) const
{
    const int originX = chunk.getWorldOriginX();
    const int originZ = chunk.getWorldOriginZ();

    // NoiseGeneratorPerlin#getRegion applies an internal /1.5 to the input
    // scale. BetaSimplexOctaves retains that behavior, so 0.0625*1.5 gives
    // the 1.12.2 public 0.0625 surface scale exactly.
    surfaceDepthNoise_.generate(
        surfaceDepthField_,
        static_cast<double>(originX),
        static_cast<double>(originZ),
        Chunk::WIDTH,
        Chunk::DEPTH,
        0.0625 * 1.5,
        0.0625 * 1.5,
        1.0,
        0.5);

    for (int localX = 0; localX < Chunk::WIDTH; ++localX)
    {
        for (int localZ = 0; localZ < Chunk::DEPTH; ++localZ)
        {
            const std::size_t index = climateIndex(localX, localZ, Chunk::DEPTH);
            surfaceBuilder_.replaceColumn(
                chunk,
                localX,
                localZ,
                climate[index],
                -1.0,
                -1.0,
                surfaceDepthField_[index],
                chunkRandom_);
        }
    }
}

void TerrainGenerator::initializeNoiseField(
    int originX,
    int originY,
    int originZ,
    int sizeX,
    int sizeY,
    int sizeZ,
    const std::vector<ClimateSample>& climate) const
{
    constexpr double coordinateScale = 684.412;
    constexpr double heightScale = 684.412;
    constexpr double mainNoiseScaleX = 80.0;
    constexpr double mainNoiseScaleY = 160.0;
    constexpr double mainNoiseScaleZ = 80.0;
    constexpr double depthNoiseScaleX = 200.0;
    constexpr double depthNoiseScaleZ = 200.0;
    constexpr double lowerLimitScale = 512.0;
    constexpr double upperLimitScale = 512.0;
    constexpr double depthNoiseScaleExponent = 0.5;
    constexpr double baseSize = 8.5;
    constexpr double stretchY = 12.0;

    // scaleNoise is constructed/consumed in vanilla but not sampled by the
    // default generateHeightmap path. Keep the member for exact RNG order.
    (void)scaleField_;

    depthNoise_.generateNoise2D(
        depthField_, originX, originZ, sizeX, sizeZ,
        depthNoiseScaleX, depthNoiseScaleZ);
    mainNoise_.generateNoiseOctaves(
        mainField_, originX, originY, originZ,
        sizeX, sizeY, sizeZ,
        coordinateScale / mainNoiseScaleX,
        heightScale / mainNoiseScaleY,
        coordinateScale / mainNoiseScaleZ);
    minLimitNoise_.generateNoiseOctaves(
        minLimitField_, originX, originY, originZ,
        sizeX, sizeY, sizeZ,
        coordinateScale, heightScale, coordinateScale);
    maxLimitNoise_.generateNoiseOctaves(
        maxLimitField_, originX, originY, originZ,
        sizeX, sizeY, sizeZ,
        coordinateScale, heightScale, coordinateScale);

    densityField_.assign(
        static_cast<std::size_t>(sizeX * sizeY * sizeZ), 0.0);

    std::array<float, 25> biomeWeights{};
    for (int offsetX = -2; offsetX <= 2; ++offsetX)
    {
        for (int offsetZ = -2; offsetZ <= 2; ++offsetZ)
        {
            biomeWeights[static_cast<std::size_t>(
                offsetX + 2 + (offsetZ + 2) * 5)] =
                10.0f / std::sqrt(
                    static_cast<float>(offsetX * offsetX + offsetZ * offsetZ) + 0.2f);
        }
    }

    int densityPosition = 0;
    int horizontalPosition = 0;
    for (int x = 0; x < sizeX; ++x)
    {
        for (int z = 0; z < sizeZ; ++z)
        {
            const ClimateSample& centre = climate[climateIndex(x + 2, z + 2, 10)];
            const BiomeDefinition* centreBiome = BiomeRegistry::active().find(centre.biome);
            const float centreDepth = centreBiome == nullptr ? 0.125f : centreBiome->baseHeight;

            float variationAverage = 0.0f;
            float depthAverage = 0.0f;
            float weightTotal = 0.0f;
            for (int offsetX = -2; offsetX <= 2; ++offsetX)
            {
                for (int offsetZ = -2; offsetZ <= 2; ++offsetZ)
                {
                    const ClimateSample& nearby = climate[climateIndex(
                        x + offsetX + 2, z + offsetZ + 2, 10)];
                    const BiomeDefinition* nearbyBiome =
                        BiomeRegistry::active().find(nearby.biome);
                    const float nearbyDepth = nearbyBiome == nullptr
                        ? 0.125f : nearbyBiome->baseHeight;
                    const float nearbyVariation = nearbyBiome == nullptr
                        ? 0.05f : nearbyBiome->heightVariation;
                    float weight = biomeWeights[static_cast<std::size_t>(
                        offsetX + 2 + (offsetZ + 2) * 5)] /
                        (nearbyDepth + 2.0f);
                    if (nearbyDepth > centreDepth)
                        weight *= 0.5f;
                    variationAverage += nearbyVariation * weight;
                    depthAverage += nearbyDepth * weight;
                    weightTotal += weight;
                }
            }

            float variation = variationAverage / weightTotal;
            float depth = depthAverage / weightTotal;
            variation = variation * 0.9f + 0.1f;
            depth = (depth * 4.0f - 1.0f) / 8.0f;

            double depthNoise = depthField_[static_cast<std::size_t>(
                horizontalPosition++)] / 8000.0;
            if (depthNoise < 0.0)
                depthNoise = -depthNoise * 0.3;
            depthNoise = depthNoise * 3.0 - 2.0;
            if (depthNoise < 0.0)
            {
                depthNoise /= 2.0;
                depthNoise = std::max(depthNoise, -1.0);
                depthNoise /= 1.4;
                depthNoise /= 2.0;
            }
            else
            {
                depthNoise = std::min(depthNoise, 1.0);
                depthNoise /= 8.0;
            }

            double biomeDepth = static_cast<double>(depth);
            biomeDepth += depthNoise * 0.2;
            biomeDepth = biomeDepth * static_cast<double>(baseSize) / 8.0;
            const double centreY = baseSize + biomeDepth * 4.0;

            for (int y = 0; y < sizeY; ++y)
            {
                double verticalShape =
                    (static_cast<double>(y) - centreY) * stretchY * 128.0 / 256.0 /
                    static_cast<double>(variation);
                if (verticalShape < 0.0)
                    verticalShape *= 4.0;

                const double minimum = minLimitField_[
                    static_cast<std::size_t>(densityPosition)] / lowerLimitScale;
                const double maximum = maxLimitField_[
                    static_cast<std::size_t>(densityPosition)] / upperLimitScale;
                const double selector = (mainField_[
                    static_cast<std::size_t>(densityPosition)] / 10.0 + 1.0) / 2.0;
                double density = selector < 0.0
                    ? minimum
                    : selector > 1.0
                        ? maximum
                        : minimum + (maximum - minimum) * selector;
                density -= verticalShape;

                if (y > 29)
                {
                    const double fade = static_cast<double>(y - 29) / 3.0;
                    density = density * (1.0 - fade) + -10.0 * fade;
                }

                densityField_[static_cast<std::size_t>(densityPosition++)] = density;
            }
        }
    }

    (void)depthNoiseScaleExponent; // default exponent is consumed in settings;
                                   // the branch above is its 0.5-specialized form.
}

long long TerrainGenerator::makeChunkSeed(int chunkX, int chunkZ) noexcept
{
    std::uint64_t value =
        static_cast<std::uint64_t>(static_cast<std::int64_t>(chunkX)) *
        341873128712ULL;
    value +=
        static_cast<std::uint64_t>(static_cast<std::int64_t>(chunkZ)) *
        132897987541ULL;
    return std::bit_cast<long long>(value);
}
