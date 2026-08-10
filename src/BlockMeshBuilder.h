#pragma once

#include "ChunkMesh.h"

[[nodiscard]] int appendBlockMesh(
    ChunkMeshData& output,
    const ChunkMeshInput& input,
    const BiomeColorMap& colourMap,
    int x,
    int y,
    int z,
    mc::content::BlockState state);
