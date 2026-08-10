#pragma once

#include "Block.h"

#include <glm/vec3.hpp>

#include <span>

enum class BlockRenderShape
{
    None,
    Boxes,
    CrossedPlanes,
    WallPlane,
    Fluid
};

struct BlockBox
{
    glm::vec3 minimum{0.0f};
    glm::vec3 maximum{1.0f};
};

struct BlockShapeDefinition
{
    BlockRenderShape renderShape = BlockRenderShape::None;
    std::span<const BlockBox> renderBoxes;
    std::span<const BlockBox> selectionBoxes;
    std::span<const BlockBox> collisionBoxes;
    bool occludesNeighbourFaces = false;
};

// Shape definitions are deliberately independent from block textures and
// materials. A future slab, stair, fence, or multipart block only needs one
// or more local 0..1 boxes here; meshing, collision, raycasting, placement,
// and selection outlines all consume the same definition.
[[nodiscard]] const BlockShapeDefinition& getBlockShape(
    BlockType block
) noexcept;

[[nodiscard]] bool boxesIntersect(
    const BlockBox& left,
    const BlockBox& right
) noexcept;

[[nodiscard]] BlockBox translateBlockBox(
    const BlockBox& box,
    const glm::ivec3& blockPosition
) noexcept;
