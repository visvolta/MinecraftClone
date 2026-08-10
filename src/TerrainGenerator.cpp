#include "TerrainGenerator.h"

#include "Chunk.h"
#include "worldgen/WorldGenerationContext.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace
{
std::size_t densityIndex(
    int x,
    int z,
    int y,
    int sizeZ,
    int sizeY)
{
    return static_cast<std::size_t>(
        (x * sizeZ + z) * sizeY + y
    );
}

std::size_t climateIndex(
    int x,
    int z,
    int depth)
{
    return static_cast<std::size_t>(
        x * depth + z
    );
}
}

TerrainGenerator::TerrainGenerator(int seed)
    : seed_(seed),
      generatorRandom_(seed),
      chunkRandom_(seed),
      minLimitNoise_(generatorRandom_, 16),
      maxLimitNoise_(generatorRandom_, 16),
      mainNoise_(generatorRandom_, 8),
      surfaceDepthNoise_(generatorRandom_, 4),
      scaleNoise_(generatorRandom_, 10),
      depthNoise_(generatorRandom_, 16),
      mobSpawnerNoise_(generatorRandom_, 8),
      biomeMap_(seed),
      caveGenerator_(seed),
      populationGenerator_(seed)
{
}

void TerrainGenerator::generateChunk(
    Chunk& chunk) const
{
    chunk.clear();

    const std::vector<ClimateSample> climate =
        biomeMap_.sampleArea(
            chunk.getWorldOriginX(),
            chunk.getWorldOriginZ(),
            Chunk::WIDTH,
            Chunk::DEPTH
        );
    const std::vector<ClimateSample> generationBiomes =
        biomeMap_.sampleGenerationArea(
            chunk.getChunkX() * 4 - 2,
            chunk.getChunkZ() * 4 - 2,
            10,
            10
        );

    for (int localX = 0;
         localX < Chunk::WIDTH;
         ++localX)
    {
        for (int localZ = 0;
             localZ < Chunk::DEPTH;
             ++localZ)
        {
            const ClimateSample& sample =
                climate[
                    climateIndex(
                        localX,
                        localZ,
                        Chunk::DEPTH
                    )
                ];

            chunk.setBiome(localX, localZ, sample.biome);
            chunk.setClimate(
                localX,
                localZ,
                static_cast<float>(sample.temperature),
                static_cast<float>(sample.humidity)
            );
        }
    }

    generateBaseTerrain(chunk, generationBiomes);

    chunkRandom_.setSeed(
        makeChunkSeed(
            chunk.getChunkX(),
            chunk.getChunkZ()
        )
    );

    replaceSurfaceBlocks(chunk, climate);

    // ChunkGeneratorOverworld carves caves after biome surface replacement.
    caveGenerator_.generate(chunk);

    WorldGenerationContext context(
        chunk,
        [this](
            int worldX,
            int worldY,
            int worldZ)
        {
            return sampleBaseBlock(
                worldX,
                worldY,
                worldZ
            );
        }
    );

    // Population replays neighbouring source chunks so trees and decorators
    // remain deterministic across independently generated chunk borders.
    populationGenerator_.populate(
        chunk,
        context
    );
}

int TerrainGenerator::getSeed() const noexcept
{
    return seed_;
}

int TerrainGenerator::getTerrainHeight(
    int,
    int) const
{
    // Kept for API compatibility. Runtime spawning uses the fully generated
    // loaded chunk through World::getHighestSolidBlockY.
    return SurfaceBuilder::SEA_LEVEL;
}

void TerrainGenerator::generateBaseTerrain(
    Chunk& chunk,
    const std::vector<ClimateSample>& climate) const
{
    constexpr int horizontalCells = 4;
    constexpr int densitySizeX = horizontalCells + 1;
    constexpr int densitySizeY = 33;
    constexpr int densitySizeZ = horizontalCells + 1;

    initializeNoiseField(
        chunk.getChunkX() * horizontalCells,
        0,
        chunk.getChunkZ() * horizontalCells,
        densitySizeX,
        densitySizeY,
        densitySizeZ,
        climate
    );

    for (int cellX = 0;
         cellX < horizontalCells;
         ++cellX)
    {
        for (int cellZ = 0;
             cellZ < horizontalCells;
             ++cellZ)
        {
            for (int densityY = 0;
                 densityY < 32;
                 ++densityY)
            {
                double density00 =
                    densityField_[
                        densityIndex(
                            cellX,
                            cellZ,
                            densityY,
                            densitySizeZ,
                            densitySizeY
                        )
                    ];

                double density01 =
                    densityField_[
                        densityIndex(
                            cellX,
                            cellZ + 1,
                            densityY,
                            densitySizeZ,
                            densitySizeY
                        )
                    ];

                double density10 =
                    densityField_[
                        densityIndex(
                            cellX + 1,
                            cellZ,
                            densityY,
                            densitySizeZ,
                            densitySizeY
                        )
                    ];

                double density11 =
                    densityField_[
                        densityIndex(
                            cellX + 1,
                            cellZ + 1,
                            densityY,
                            densitySizeZ,
                            densitySizeY
                        )
                    ];

                const double deltaY00 =
                    (densityField_[
                         densityIndex(
                             cellX,
                             cellZ,
                             densityY + 1,
                             densitySizeZ,
                             densitySizeY
                         )
                     ] -
                     density00) /
                    8.0;

                const double deltaY01 =
                    (densityField_[
                         densityIndex(
                             cellX,
                             cellZ + 1,
                             densityY + 1,
                             densitySizeZ,
                             densitySizeY
                         )
                     ] -
                     density01) /
                    8.0;

                const double deltaY10 =
                    (densityField_[
                         densityIndex(
                             cellX + 1,
                             cellZ,
                             densityY + 1,
                             densitySizeZ,
                             densitySizeY
                         )
                     ] -
                     density10) /
                    8.0;

                const double deltaY11 =
                    (densityField_[
                         densityIndex(
                             cellX + 1,
                             cellZ + 1,
                             densityY + 1,
                             densitySizeZ,
                             densitySizeY
                         )
                     ] -
                     density11) /
                    8.0;

                for (int subY = 0;
                     subY < 8;
                     ++subY)
                {
                    double xStart0 = density00;
                    double xStart1 = density01;

                    const double deltaX0 =
                        (density10 - density00) /
                        4.0;

                    const double deltaX1 =
                        (density11 - density01) /
                        4.0;

                    for (int subX = 0;
                         subX < 4;
                         ++subX)
                    {
                        double density = xStart0;
                        const double deltaZ =
                            (xStart1 - xStart0) /
                            4.0;

                        for (int subZ = 0;
                             subZ < 4;
                             ++subZ)
                        {
                            const int localX =
                                cellX * 4 + subX;
                            const int localY =
                                densityY * 8 + subY;
                            const int localZ =
                                cellZ * 4 + subZ;

                            BlockType block =
                                BlockType::Air;

                            if (density > 0.0)
                            {
                                block = BlockType::Stone;
                            }
                            else if (
                                localY <
                                SurfaceBuilder::SEA_LEVEL)
                            {
                                block = BlockType::Water;
                            }

                            chunk.setBlock(
                                localX,
                                localY,
                                localZ,
                                block
                            );

                            density += deltaZ;
                        }

                        xStart0 += deltaX0;
                        xStart1 += deltaX1;
                    }

                    density00 += deltaY00;
                    density01 += deltaY01;
                    density10 += deltaY10;
                    density11 += deltaY11;
                }
            }
        }
    }
}

void TerrainGenerator::replaceSurfaceBlocks(
    Chunk& chunk,
    const std::vector<ClimateSample>& climate) const
{
    const int originX =
        chunk.getWorldOriginX();
    const int originZ =
        chunk.getWorldOriginZ();

    surfaceDepthNoise_.generateNoiseOctaves(
        surfaceDepthField_,
        static_cast<double>(originX),
        static_cast<double>(originZ),
        0.0,
        Chunk::WIDTH,
        Chunk::DEPTH,
        1,
        0.0625,
        0.0625,
        0.0625
    );

    for (int localX = 0;
         localX < Chunk::WIDTH;
         ++localX)
    {
        for (int localZ = 0;
             localZ < Chunk::DEPTH;
             ++localZ)
        {
            const std::size_t index =
                climateIndex(
                    localX,
                    localZ,
                    Chunk::DEPTH
                );

            surfaceBuilder_.replaceColumn(
                chunk,
                localX,
                localZ,
                climate[index],
                -1.0,
                -1.0,
                surfaceDepthField_[index],
                chunkRandom_
            );
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
    constexpr double horizontalScale = 684.412;
    constexpr double verticalScale = 684.412;

    scaleNoise_.generateNoise2D(
        scaleField_,
        originX,
        originZ,
        sizeX,
        sizeZ,
        1.121,
        1.121
    );

    depthNoise_.generateNoise2D(
        depthField_,
        originX,
        originZ,
        sizeX,
        sizeZ,
        200.0,
        200.0
    );

    mainNoise_.generateNoiseOctaves(
        mainField_,
        static_cast<double>(originX),
        static_cast<double>(originY),
        static_cast<double>(originZ),
        sizeX,
        sizeY,
        sizeZ,
        horizontalScale / 80.0,
        verticalScale / 160.0,
        horizontalScale / 80.0
    );

    minLimitNoise_.generateNoiseOctaves(
        minLimitField_,
        static_cast<double>(originX),
        static_cast<double>(originY),
        static_cast<double>(originZ),
        sizeX,
        sizeY,
        sizeZ,
        horizontalScale,
        verticalScale,
        horizontalScale
    );

    maxLimitNoise_.generateNoiseOctaves(
        maxLimitField_,
        static_cast<double>(originX),
        static_cast<double>(originY),
        static_cast<double>(originZ),
        sizeX,
        sizeY,
        sizeZ,
        horizontalScale,
        verticalScale,
        horizontalScale
    );

    densityField_.assign(
        static_cast<std::size_t>(
            sizeX * sizeY * sizeZ
        ),
        0.0
    );

    int densityPosition = 0;
    int horizontalPosition = 0;
    std::array<float, 25> biomeWeights{};
    for (int offsetX = -2; offsetX <= 2; ++offsetX)
    {
        for (int offsetZ = -2; offsetZ <= 2; ++offsetZ)
        {
            biomeWeights[static_cast<std::size_t>(
                offsetX + 2 + (offsetZ + 2) * 5
            )] = 10.0f / std::sqrt(
                static_cast<float>(offsetX * offsetX + offsetZ * offsetZ) +
                0.2f
            );
        }
    }

    for (int x = 0; x < sizeX; ++x)
    {
        for (int z = 0; z < sizeZ; ++z)
        {
            const ClimateSample& centre = climate[climateIndex(
                x + 2, z + 2, 10
            )];
            const BiomeDefinition* centreBiome =
                BiomeRegistry::active().find(centre.biome);
            const float centreDepth = centreBiome == nullptr
                ? 0.125f
                : centreBiome->baseHeight;
            float scale = 0.0f;
            float depthAverage = 0.0f;
            float weightTotal = 0.0f;
            for (int offsetX = -2; offsetX <= 2; ++offsetX)
            {
                for (int offsetZ = -2; offsetZ <= 2; ++offsetZ)
                {
                    const ClimateSample& nearby = climate[climateIndex(
                        x + offsetX + 2,
                        z + offsetZ + 2,
                        10
                    )];
                    const BiomeDefinition* nearbyBiome =
                        BiomeRegistry::active().find(nearby.biome);
                    const float nearbyDepth = nearbyBiome == nullptr
                        ? 0.125f
                        : nearbyBiome->baseHeight;
                    const float nearbyScale = nearbyBiome == nullptr
                        ? 0.05f
                        : nearbyBiome->heightVariation;
                    float weight = biomeWeights[static_cast<std::size_t>(
                        offsetX + 2 + (offsetZ + 2) * 5
                    )] / (nearbyDepth + 2.0f);
                    if (nearbyDepth > centreDepth)
                        weight *= 0.5f;
                    scale += nearbyScale * weight;
                    depthAverage += nearbyDepth * weight;
                    weightTotal += weight;
                }
            }
            scale = scale / weightTotal * 0.9f + 0.1f;
            double baseDepth =
                (depthAverage / weightTotal * 4.0f - 1.0f) / 8.0f;

            double depthNoise =
                depthField_[
                    static_cast<std::size_t>(
                        horizontalPosition
                    )
                ] /
                8000.0;

            if (depthNoise < 0.0)
            {
                depthNoise = -depthNoise * 0.3;
            }

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
            baseDepth += depthNoise * 0.2;
            baseDepth *= 8.5 / 8.0;
            const double centreY = 8.5 + baseDepth * 4.0;

            ++horizontalPosition;

            for (int y = 0;
                 y < sizeY;
                 ++y)
            {
                double verticalShape =
                    (static_cast<double>(y) -
                     centreY) *
                    12.0 * 128.0 / 256.0 /
                    static_cast<double>(scale);

                if (verticalShape < 0.0)
                {
                    verticalShape *= 4.0;
                }

                const double minimum =
                    minLimitField_[
                        static_cast<std::size_t>(
                            densityPosition
                        )
                    ] /
                    512.0;

                const double maximum =
                    maxLimitField_[
                        static_cast<std::size_t>(
                            densityPosition
                        )
                    ] /
                    512.0;

                const double selector =
                    (mainField_[
                         static_cast<std::size_t>(
                             densityPosition
                         )
                     ] /
                         10.0 +
                     1.0) /
                    2.0;

                double density = 0.0;

                if (selector < 0.0)
                {
                    density = minimum;
                }
                else if (selector > 1.0)
                {
                    density = maximum;
                }
                else
                {
                    density =
                        minimum +
                        (maximum - minimum) *
                            selector;
                }

                density -= verticalShape;

                if (y > sizeY - 4)
                {
                    const double fade =
                        static_cast<double>(
                            y - (sizeY - 4)
                        ) /
                        3.0;

                    density =
                        density * (1.0 - fade) +
                        -10.0 * fade;
                }

                densityField_[
                    static_cast<std::size_t>(
                        densityPosition
                    )
                ] = density;

                ++densityPosition;
            }
        }
    }
}

BlockType TerrainGenerator::sampleBaseBlock(
    int,
    int worldY,
    int) const
{
    if (worldY < 0 ||
        worldY >= Chunk::HEIGHT)
    {
        return BlockType::Air;
    }

    if (worldY == 0)
    {
        return BlockType::Bedrock;
    }

    // Used only when validating a feature just outside the target chunk.
    // This conservative sea-level profile keeps independently generated
    // border features deterministic without touching live neighbour chunks.
    if (worldY < SurfaceBuilder::SEA_LEVEL - 4)
    {
        return BlockType::Stone;
    }

    if (worldY < SurfaceBuilder::SEA_LEVEL)
    {
        return BlockType::Water;
    }

    return BlockType::Air;
}

long long TerrainGenerator::makeChunkSeed(
    int chunkX,
    int chunkZ) noexcept
{
    std::uint64_t value =
        static_cast<std::uint64_t>(
            static_cast<std::int64_t>(chunkX)
        ) *
        341873128712ULL;

    value +=
        static_cast<std::uint64_t>(
            static_cast<std::int64_t>(chunkZ)
        ) *
        132897987541ULL;

    return std::bit_cast<long long>(value);
}
