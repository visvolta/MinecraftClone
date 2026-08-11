#include "ChunkMeshing.h"

#include "BlockShape.h"
#include "content/BlockStateLogic.h"
#include "content/ContentCatalog.h"

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

mc::content::BlockState actualState(
    const ChunkMeshInput& input,
    int x,
    int y,
    int z,
    mc::content::BlockState state)
{
    return mc::content::resolveActualBlockState(
        state,
        {{
            sampleState(input, x, y, z - 1),
            sampleState(input, x + 1, y, z),
            sampleState(input, x, y, z + 1),
            sampleState(input, x - 1, y, z)
        }},
        sampleState(input, x, y + 1, z)
    );
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

bool occludesNeighbourFace(mc::content::BlockState state) noexcept
{
    if (state.isAir())
        return false;
    if (const mc::content::ContentCatalog* catalog =
            mc::content::ContentCatalog::active())
    {
        if (const mc::content::BlockDefinition* definition = catalog->block(state))
        {
            static_cast<void>(definition);
            return getBlockShape(state).occludesNeighbourFaces;
        }
    }
    return getBlockShape(state).occludesNeighbourFaces;
}

bool shouldRenderFace(
    mc::content::BlockState block,
    mc::content::BlockState neighbour) noexcept
{
    if (neighbour.isAir())
        return true;
    const mc::content::ContentCatalog* catalog =
        mc::content::ContentCatalog::active();
    const mc::content::BlockDefinition* blockDefinition =
        catalog == nullptr ? nullptr : catalog->block(block);
    const mc::content::BlockDefinition* neighbourDefinition =
        catalog == nullptr ? nullptr : catalog->block(neighbour);
    if (blockDefinition == nullptr || neighbourDefinition == nullptr)
        return shouldRenderFace(block.block(), neighbour.block());
    if (block.blockRuntimeId() == neighbour.blockRuntimeId() &&
        blockDefinition->behaviour.traits.translucent)
        return false;
    if (blockDefinition->behaviour.traits.opaque)
        return !occludesNeighbourFace(neighbour);
    if (blockDefinition->behaviour.traits.cutout ||
        blockDefinition->behaviour.traits.leaf)
        return !neighbourDefinition->behaviour.traits.opaque;
    if (blockDefinition->behaviour.traits.translucent)
        return neighbour.isAir() || neighbourDefinition->behaviour.traits.cutout;
    return !occludesNeighbourFace(neighbour);
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
