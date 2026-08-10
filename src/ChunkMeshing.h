#pragma once

#include "ChunkMesh.h"

#include <cstdint>

namespace ChunkMeshing
{
[[nodiscard]] mc::content::BlockState sampleState(
    const ChunkMeshInput& input,
    int x,
    int y,
    int z
) noexcept;

[[nodiscard]] BlockType sampleBlock(
    const ChunkMeshInput& input,
    int x,
    int y,
    int z) noexcept;

[[nodiscard]] int sampleLight(
    const ChunkMeshInput& input,
    int x,
    int y,
    int z) noexcept;

[[nodiscard]] float classicBrightness(int lightLevel) noexcept;

[[nodiscard]] bool shouldRenderFace(
    BlockType block,
    BlockType neighbour) noexcept;

[[nodiscard]] bool shouldRenderFastLeafFace(
    BlockType leaf,
    BlockType neighbour) noexcept;

[[nodiscard]] bool shouldRenderFancyLeafFace(
    BlockType neighbour) noexcept;

[[nodiscard]] bool shouldRenderFluidFace(
    BlockType liquid,
    BlockType neighbour) noexcept;
}
