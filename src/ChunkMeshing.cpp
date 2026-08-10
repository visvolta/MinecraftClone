#include "ChunkMeshing.h"

#include "BlockShape.h"

#include <algorithm>

namespace ChunkMeshing
{
mc::content::BlockState sampleState(
    const ChunkMeshInput& input,
    int x,
    int y,
    int z) noexcept
{
    if (!input.snapshot || y < 0 || y >= Chunk::HEIGHT ||
        x < -ChunkMeshSnapshot::Border || x >= Chunk::WIDTH + ChunkMeshSnapshot::Border ||
        z < -ChunkMeshSnapshot::Border || z >= Chunk::DEPTH + ChunkMeshSnapshot::Border)
    {
        return {};
    }
    return input.snapshot->getBlockState(x, y, z);
}

BlockType sampleBlock(
    const ChunkMeshInput& input,
    int x,
    int y,
    int z) noexcept
{
    return sampleState(input, x, y, z).block();
}

int sampleLight(
    const ChunkMeshInput& input,
    int x,
    int y,
    int z) noexcept
{
    if (y < 0)
        return 0;
    if (y >= Chunk::HEIGHT)
        return 15;

    if (!input.snapshot)
        return 0;
    const int clampedX = std::clamp(
        x, -ChunkMeshSnapshot::Border,
        Chunk::WIDTH + ChunkMeshSnapshot::Border - 1
    );
    const int clampedZ = std::clamp(
        z, -ChunkMeshSnapshot::Border,
        Chunk::DEPTH + ChunkMeshSnapshot::Border - 1
    );
    return std::max<int>(
        input.snapshot->getSkyLight(clampedX, y, clampedZ),
        input.snapshot->getBlockLight(clampedX, y, clampedZ)
    );
}

float classicBrightness(int lightLevel) noexcept
{
    lightLevel = std::clamp(lightLevel, 0, 15);
    constexpr float ambientFloor = 0.05f;
    const float darkness = 1.0f - static_cast<float>(lightLevel) / 15.0f;
    return (1.0f - darkness) / (darkness * 3.0f + 1.0f) *
           (1.0f - ambientFloor) + ambientFloor;
}

bool shouldRenderFace(
    BlockType block,
    BlockType neighbour) noexcept
{
    if (isAir(neighbour)) return true;
    if (isLeaf(block) && isLeaf(neighbour)) return false;
    if (block == neighbour && isTranslucent(block)) return false;
    if (isOpaque(block))
        return !getBlockShape(neighbour).occludesNeighbourFaces;
    if (isCutout(block)) return !isOpaque(neighbour);
    if (isTranslucent(block)) return isAir(neighbour) || isCutout(neighbour);
    return false;
}

bool shouldRenderFastLeafFace(
    BlockType leaf,
    BlockType neighbour) noexcept
{
    // Fast graphics treats a run of leaves as an opaque volume and removes
    // internal faces. Different leaf species still share that volume.
    return isLeaf(leaf) && !isLeaf(neighbour) &&
           !getBlockShape(neighbour).occludesNeighbourFaces;
}

bool shouldRenderFancyLeafFace(BlockType neighbour) noexcept
{
    // Fancy graphics preserves faces between transparent leaf blocks, which
    // is the classic Beta behaviour. Fully opaque neighbours still cull.
    return isLeaf(neighbour) ||
           !getBlockShape(neighbour).occludesNeighbourFaces;
}

bool shouldRenderFluidFace(
    BlockType liquid,
    BlockType neighbour) noexcept
{
    return neighbour != liquid &&
           !getBlockShape(neighbour).occludesNeighbourFaces;
}
}
