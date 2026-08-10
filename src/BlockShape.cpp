#include "BlockShape.h"

#include "content/ContentCatalog.h"

#include <array>

namespace
{
constexpr std::array<BlockBox, 1> FullCube{{
    {{0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}}
}};

constexpr std::array<BlockBox, 1> FlowerSelection{{
    {{0.3f, 0.0f, 0.3f}, {0.7f, 0.6f, 0.7f}}
}};

constexpr std::array<BlockBox, 1> MushroomSelection{{
    {{0.3f, 0.0f, 0.3f}, {0.7f, 0.4f, 0.7f}}
}};

constexpr std::array<BlockBox, 1> LadderSelection{{
    {{0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.125f}}
}};

constexpr std::array<BlockBox, 1> FarmlandBoxes{{
    {{0.0f, 0.0f, 0.0f}, {1.0f, 0.9375f, 1.0f}}
}};

constexpr std::array<BlockBox, 1> CactusBoxes{{
    {{0.0625f, 0.0f, 0.0625f}, {0.9375f, 1.0f, 0.9375f}}
}};

constexpr std::array<BlockBox, 1> WireBoxes{{
    {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0625f, 1.0f}}
}};

constexpr std::array<BlockBox, 1> ComponentBoxes{{
    {{0.125f, 0.0f, 0.125f}, {0.875f, 0.25f, 0.875f}}
}};

const BlockShapeDefinition EmptyShape{
    BlockRenderShape::None,
    {},
    {},
    {},
    false
};

const BlockShapeDefinition CubeShape{
    BlockRenderShape::Boxes,
    FullCube,
    FullCube,
    FullCube,
    true
};

const BlockShapeDefinition TransparentCubeShape{
    BlockRenderShape::Boxes,
    FullCube,
    FullCube,
    FullCube,
    false
};

const BlockShapeDefinition FlowerShape{
    BlockRenderShape::CrossedPlanes,
    {},
    FlowerSelection,
    {},
    false
};

const BlockShapeDefinition MushroomShape{
    BlockRenderShape::CrossedPlanes,
    {},
    MushroomSelection,
    {},
    false
};

const BlockShapeDefinition LadderShape{
    BlockRenderShape::WallPlane,
    {},
    LadderSelection,
    {},
    false
};

const BlockShapeDefinition FluidShape{
    BlockRenderShape::Fluid,
    {},
    {},
    {},
    false
};

const BlockShapeDefinition FarmlandShape{
    BlockRenderShape::Boxes,
    FarmlandBoxes,
    FarmlandBoxes,
    FarmlandBoxes,
    false
};

const BlockShapeDefinition CactusShape{
    BlockRenderShape::Boxes,
    CactusBoxes,
    CactusBoxes,
    CactusBoxes,
    false
};

const BlockShapeDefinition WireShape{
    BlockRenderShape::Boxes,
    WireBoxes,
    WireBoxes,
    {},
    false
};

const BlockShapeDefinition ComponentShape{
    BlockRenderShape::Boxes,
    ComponentBoxes,
    ComponentBoxes,
    {},
    false
};
}

const BlockShapeDefinition& getBlockShape(BlockType block) noexcept
{
    if (const mc::content::ContentCatalog* catalog =
            mc::content::ContentCatalog::active())
    {
        if (const mc::content::BlockDefinition* definition =
                catalog->block(block);
            definition != nullptr && definition->behaviour.shape != nullptr)
        {
            return *definition->behaviour.shape;
        }
    }
    if (block == BlockType::Air)
        return EmptyShape;
    if (isLiquid(block))
        return FluidShape;
    if (isLeaf(block))
        return TransparentCubeShape;
    if (isTranslucent(block) && !isLiquid(block))
        return TransparentCubeShape;
    if (block == BlockType::Farmland)
        return FarmlandShape;
    if (block == BlockType::Cactus)
        return CactusShape;
    if (block == BlockType::RedstoneWire)
        return WireShape;
    if (block == BlockType::Lever || block == BlockType::Repeater)
        return ComponentShape;
    if (block == BlockType::RedstoneTorch)
        return FlowerShape;
    if (block == BlockType::BrownMushroom ||
        block == BlockType::RedMushroom)
    {
        return MushroomShape;
    }
    if (isPlant(block))
        return FlowerShape;
    if (isLadder(block))
        return LadderShape;
    return CubeShape;
}

bool boxesIntersect(
    const BlockBox& left,
    const BlockBox& right) noexcept
{
    return left.maximum.x > right.minimum.x &&
           left.minimum.x < right.maximum.x &&
           left.maximum.y > right.minimum.y &&
           left.minimum.y < right.maximum.y &&
           left.maximum.z > right.minimum.z &&
           left.minimum.z < right.maximum.z;
}

BlockBox translateBlockBox(
    const BlockBox& box,
    const glm::ivec3& blockPosition) noexcept
{
    const glm::vec3 offset(blockPosition);
    return {box.minimum + offset, box.maximum + offset};
}
