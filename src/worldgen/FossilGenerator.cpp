#include "worldgen/FossilGenerator.h"

#include "Chunk.h"
#include "worldgen/JavaRandom.h"
#include "worldgen/WorldGenerationContext.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <string>
#include <string_view>

namespace mc112
{
namespace
{
std::int32_t javaIntMul(std::int32_t first, std::int32_t second) noexcept
{
    const std::uint32_t value =
        static_cast<std::uint32_t>(first) * static_cast<std::uint32_t>(second);
    return std::bit_cast<std::int32_t>(value);
}

std::int64_t javaLongAdd(std::int64_t first, std::int64_t second) noexcept
{
    const std::uint64_t value =
        static_cast<std::uint64_t>(first) + static_cast<std::uint64_t>(second);
    return std::bit_cast<std::int64_t>(value);
}

std::int64_t javaLongMul(std::int64_t first, std::int64_t second) noexcept
{
    const std::uint64_t value =
        static_cast<std::uint64_t>(first) * static_cast<std::uint64_t>(second);
    return std::bit_cast<std::int64_t>(value);
}

std::int64_t fossilChunkSeed(
    std::int64_t worldSeed,
    int chunkX,
    int chunkZ) noexcept
{
    // Chunk#getRandomWithSeed. Preserve Java's int overflow at every cast
    // boundary in the original expression.
    const std::int32_t x = static_cast<std::int32_t>(chunkX);
    const std::int32_t z = static_cast<std::int32_t>(chunkZ);

    const std::int32_t xSquared = javaIntMul(x, x);
    const std::int32_t xTerm1 = javaIntMul(xSquared, 4987142);
    const std::int32_t xTerm2 = javaIntMul(x, 5947611);
    const std::int32_t zSquared = javaIntMul(z, z);
    const std::int64_t zTerm1 = javaLongMul(
        static_cast<std::int64_t>(zSquared), 4392871LL);
    const std::int32_t zTerm2 = javaIntMul(z, 389711);

    std::int64_t seed = worldSeed;
    seed = javaLongAdd(seed, static_cast<std::int64_t>(xTerm1));
    seed = javaLongAdd(seed, static_cast<std::int64_t>(xTerm2));
    seed = javaLongAdd(seed, zTerm1);
    seed = javaLongAdd(seed, static_cast<std::int64_t>(zTerm2));
    return std::bit_cast<std::int64_t>(
        static_cast<std::uint64_t>(seed) ^ 987234911ULL);
}

Rotation rotationByIndex(int index) noexcept
{
    switch(index)
    {
        case 1:return Rotation::Clockwise90;
        case 2:return Rotation::Clockwise180;
        case 3:return Rotation::CounterClockwise90;
        default:return Rotation::None;
    }
}

constexpr std::array<std::string_view,8> Fossils{{
    "fossils/fossil_spine_01",
    "fossils/fossil_spine_02",
    "fossils/fossil_spine_03",
    "fossils/fossil_spine_04",
    "fossils/fossil_skull_01",
    "fossils/fossil_skull_02",
    "fossils/fossil_skull_03",
    "fossils/fossil_skull_04"
}};
constexpr std::array<std::string_view,8> FossilsCoal{{
    "fossils/fossil_spine_01_coal",
    "fossils/fossil_spine_02_coal",
    "fossils/fossil_spine_03_coal",
    "fossils/fossil_spine_04_coal",
    "fossils/fossil_skull_01_coal",
    "fossils/fossil_skull_02_coal",
    "fossils/fossil_skull_03_coal",
    "fossils/fossil_skull_04_coal"
}};
}

FossilGenerator::FossilGenerator(std::int64_t worldSeed)
    : worldSeed_(worldSeed)
{
}

bool FossilGenerator::generate(
    WorldGenerationContext& context,
    int sourceChunkX,
    int sourceChunkZ) const
{
    JavaRandom random(fossilChunkSeed(worldSeed_, sourceChunkX, sourceChunkZ));
    const Rotation rotation = rotationByIndex(random.nextInt(4));
    const int index = random.nextInt(static_cast<int>(Fossils.size()));

    const StructureTemplate& fossil = templates_.get(
        std::string(Fossils[static_cast<std::size_t>(index)]));
    const StructureTemplate& coal = templates_.get(
        std::string(FossilsCoal[static_cast<std::size_t>(index)]));

    const int originX = sourceChunkX * 16;
    const int originZ = sourceChunkZ * 16;
    const Box clip{
        originX, 0, originZ,
        originX + 15, Chunk::HEIGHT, originZ + 15};

    const auto size = fossil.transformedSize(rotation);
    const int xOffset = random.nextInt(16 - size[0]);
    const int zOffset = random.nextInt(16 - size[2]);

    int minimumHeight = Chunk::HEIGHT;
    // This deliberately uses transformed X for BOTH loops. That is what
    // WorldGenFossils 1.12.2 does, including for non-square templates.
    for(int x = 0; x < size[0]; ++x)
    {
        for(int z = 0; z < size[0]; ++z)
        {
            minimumHeight = std::min(
                minimumHeight,
                context.getHeightValue(
                    originX + x + xOffset,
                    originZ + z + zOffset));
        }
    }

    const int y = std::max(
        minimumHeight - 15 - random.nextInt(10),
        10);
    const auto zero = fossil.getZeroPositionWithTransform(
        originX + xOffset,
        y,
        originZ + zOffset,
        rotation);

    fossil.place(
        context,
        zero[0], zero[1], zero[2],
        rotation,
        clip,
        0.9F,
        &random);
    coal.place(
        context,
        zero[0], zero[1], zero[2],
        rotation,
        clip,
        0.1F,
        &random);
    return true;
}
}
