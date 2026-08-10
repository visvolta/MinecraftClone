#include "ChunkMesh.h"
#include "ChunkVertex.h"
#include "LightingSystem.h"
#include "game/GameBootstrap.h"

#include <cassert>
#include <memory>

int main()
{
    static_assert(sizeof(ChunkVertex) == 32);
    static_assert(
        ChunkMeshSnapshot::SampleCount < Chunk::BLOCK_COUNT * 2U
    );

    mc::game::GameBootstrap bootstrap;
    bootstrap.loadContentModules();
    bootstrap.freezeRegistries();

    ChunkManager chunks;
    auto centre = std::make_unique<Chunk>(0, 0);
    centre->clear();
    centre->setBlock(8, 40, 8, BlockType::Stone);
    centre->setBlock(4, 50, 4, BlockType::Lava);
    chunks.insertChunk(std::move(centre));

    LightingSystem lighting;
    lighting.queueChunk(0, 0);
    const auto updated = lighting.process(chunks, 1000.0);
    assert(updated.size() == 1);
    assert(updated.front().changed);

    const Chunk* lit = chunks.getChunk(0, 0);
    assert(lit != nullptr);
    assert(lit->getSkyLight(8, Chunk::HEIGHT - 1, 8) == 15);
    assert(lit->getSkyLight(8, 40, 8) == 0);
    assert(lit->getBlockLight(4, 50, 4) == 15);
    assert(lit->getBlockLight(5, 50, 4) == 14);

    // Rebuilding an unchanged chunk must not invalidate its mesh again.
    lighting.queueChunk(0, 0);
    const auto unchanged = lighting.process(chunks, 1000.0);
    assert(unchanged.size() == 1);
    assert(!unchanged.front().changed);
}
