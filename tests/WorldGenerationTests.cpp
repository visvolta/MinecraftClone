#include "Chunk.h"
#include "TerrainGenerator.h"

#include <cassert>

namespace
{
void assertChunksEqual(const Chunk& left, const Chunk& right)
{
    assert(left.getChunkX() == right.getChunkX());
    assert(left.getChunkZ() == right.getChunkZ());
    for (int x = 0; x < Chunk::WIDTH; ++x)
    {
        for (int z = 0; z < Chunk::DEPTH; ++z)
        {
            assert(left.getBiome(x, z) == right.getBiome(x, z));
            assert(left.getTemperature(x, z) == right.getTemperature(x, z));
            assert(left.getHumidity(x, z) == right.getHumidity(x, z));
            for (int y = 0; y < Chunk::HEIGHT; ++y)
                assert(left.getBlockState(x, y, z) ==
                       right.getBlockState(x, y, z));
        }
    }
}

bool containsBlock(const Chunk& chunk, BlockType block)
{
    for (int x = 0; x < Chunk::WIDTH; ++x)
        for (int z = 0; z < Chunk::DEPTH; ++z)
            for (int y = 0; y < Chunk::HEIGHT; ++y)
                if (chunk.getBlock(x, y, z) == block)
                    return true;
    return false;
}

int blockToChunk(int coordinate)
{
    return coordinate >= 0 ? coordinate / 16 : (coordinate - 15) / 16;
}
}

int main()
{
    constexpr int seed = 8675309;
    TerrainGenerator forward(seed);
    Chunk forwardWest(0, 0);
    Chunk forwardEast(1, 0);
    forward.generateChunk(forwardWest);
    forward.generateChunk(forwardEast);

    // Generating the same border in reverse order must not change either
    // chunk. This catches decoration or structures accidentally reading a
    // previously populated neighbour rather than the terrain-only snapshot.
    TerrainGenerator reverse(seed);
    Chunk reverseEast(1, 0);
    Chunk reverseWest(0, 0);
    reverse.generateChunk(reverseEast);
    reverse.generateChunk(reverseWest);
    assertChunksEqual(forwardWest, reverseWest);
    assertChunksEqual(forwardEast, reverseEast);

    const auto villageA = forward.findNearestStructure(
        WorldStructure::Village, 0, 0);
    const auto villageB = reverse.findNearestStructure(
        WorldStructure::Village, 0, 0);
    assert(villageA && villageB);
    assert(villageA->blockX == villageB->blockX);
    assert(villageA->blockZ == villageB->blockZ);
    assert(villageA->biome == villageB->biome);

    const auto temple = forward.findNearestStructure(
        WorldStructure::Temple, 0, 0);
    const auto mineshaft = forward.findNearestStructure(
        WorldStructure::Mineshaft, 0, 0);
    assert(temple);
    assert(mineshaft);

    Chunk templeChunk(
        blockToChunk(temple->blockX), blockToChunk(temple->blockZ));
    forward.generateChunk(templeChunk);
    assert(containsBlock(templeChunk, BlockType::Chest));

    Chunk mineshaftChunk(
        blockToChunk(mineshaft->blockX), blockToChunk(mineshaft->blockZ));
    forward.generateChunk(mineshaftChunk);
    assert(containsBlock(mineshaftChunk, BlockType::Chest));
    assert(TerrainGenerator::CURRENT_GENERATION_VERSION == 3);
    return 0;
}
