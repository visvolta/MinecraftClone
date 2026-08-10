#include "Chunk.h"
#include "content/BlockState.h"

#include <cassert>

int main()
{
    using mc::content::BlockState;

    static_assert(sizeof(mc::world::PalettedBlockStorage<1>::PaletteIndex) == 2);

    const BlockState furnace(BlockType::Furnace, 5);
    assert(furnace.block() == BlockType::Furnace);
    assert(furnace.properties() == 5);
    assert(BlockState(BlockType::Water, 31).properties() == 15);

    Chunk chunk(2, -3);
    assert(chunk.getBlockState(1, 64, 1).isAir());
    assert(chunk.setBlockState(1, 64, 1, furnace));
    assert(chunk.getBlock(1, 64, 1) == BlockType::Furnace);
    assert(chunk.getMetadata(1, 64, 1) == 5);
    assert(!chunk.setBlockState(1, 64, 1, furnace));

    const ChunkSnapshot snapshot = chunk.snapshot();
    assert(snapshot.palette.size() == 2);
    assert(snapshot.paletteIndices[
        static_cast<std::size_t>(1 + Chunk::WIDTH * (1 + Chunk::DEPTH * 64))
    ] == 1);
    Chunk restored;
    restored.restore(snapshot);
    assert(restored.getChunkX() == 2);
    assert(restored.getChunkZ() == -3);
    assert(restored.getBlockState(1, 64, 1) == furnace);

    assert(restored.setMetadata(1, 64, 1, 3));
    assert(restored.getBlockState(1, 64, 1) ==
           BlockState(BlockType::Furnace, 3));
}
