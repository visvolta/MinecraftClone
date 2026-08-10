#pragma once

#include "ChunkMesh.h"

[[nodiscard]] int appendFluidMesh(
    ChunkMeshData& output,
    const ChunkMeshInput& input,
    int x,
    int y,
    int z,
    BlockType liquid);
