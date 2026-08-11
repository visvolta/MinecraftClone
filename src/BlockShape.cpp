#include "BlockShape.h"

#include "content/ContentCatalog.h"

#include <array>
#include <algorithm>
#include <vector>

#include <glm/common.hpp>
#include <glm/vector_relational.hpp>

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

constexpr std::array<BlockBox, 1> SnowBoxes{{
    {{0.0f, 0.0f, 0.0f}, {1.0f, 0.125f, 1.0f}}
}};

constexpr std::array<BlockBox, 1> WireBoxes{{
    {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0625f, 1.0f}}
}};

constexpr std::array<BlockBox, 1> ComponentBoxes{{
    {{0.125f, 0.0f, 0.125f}, {0.875f, 0.25f, 0.875f}}
}};

constexpr std::array<BlockBox, 1> ChestBoxes{{
    {{0.0625f, 0.0f, 0.0625f}, {0.9375f, 0.875f, 0.9375f}}
}};

constexpr std::array<BlockBox, 1> SoulSandBoxes{{
    {{0.0f, 0.0f, 0.0f}, {1.0f, 0.875f, 1.0f}}
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

const BlockShapeDefinition SnowShape{
    BlockRenderShape::Boxes,
    SnowBoxes,
    SnowBoxes,
    {},
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

const BlockShapeDefinition ChestShape{
    BlockRenderShape::Boxes,
    ChestBoxes,
    ChestBoxes,
    ChestBoxes,
    false
};

const BlockShapeDefinition SoulSandShape{
    BlockRenderShape::Boxes,
    SoulSandBoxes,
    SoulSandBoxes,
    SoulSandBoxes,
    false
};

struct OwnedShape
{
    std::vector<BlockBox> render;
    std::vector<BlockBox> selection;
    std::vector<BlockBox> collision;
    BlockShapeDefinition definition;

    void refresh(bool occludes)
    {
        definition = {
            BlockRenderShape::Boxes,
            render,
            selection,
            collision,
            occludes
        };
    }
};

std::vector<std::vector<OwnedShape>> ModelShapes;
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
    if (block == BlockType::Chest)
        return ChestShape;
    if (block == BlockType::SoulSand)
        return SoulSandShape;
    if (block == BlockType::Snow)
        return SnowShape;
    if (block == BlockType::RedstoneWire)
        return WireShape;
    if (block == BlockType::Lever || block == BlockType::Repeater)
        return ComponentShape;
    if (block == BlockType::Cocoa)
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

const BlockShapeDefinition& getBlockShape(
    mc::content::BlockState state) noexcept
{
    const std::size_t block = state.blockRuntimeId();
    const std::size_t properties = state.properties();
    if (block < ModelShapes.size() && properties < ModelShapes[block].size() &&
        !ModelShapes[block][properties].selection.empty())
    {
        return ModelShapes[block][properties].definition;
    }
    if (const mc::content::ContentCatalog* catalog =
            mc::content::ContentCatalog::active())
    {
        if (const mc::content::BlockDefinition* definition =
                catalog->block(state);
            definition != nullptr && definition->behaviour.shape != nullptr)
        {
            return *definition->behaviour.shape;
        }
    }
    return getBlockShape(state.block());
}

void registerModelBlockShape(
    mc::content::BlockState state,
    std::span<const BlockBox> elementBoxes,
    ModelBlockShapeKind kind,
    bool occludesNeighbourFaces)
{
    const std::size_t block = state.blockRuntimeId();
    const std::size_t properties = state.properties();
    if (ModelShapes.size() <= block)
        ModelShapes.resize(block + 1U);
    if (ModelShapes[block].size() <= properties)
        ModelShapes[block].resize(properties + 1U);

    OwnedShape& shape = ModelShapes[block][properties];
    shape.render.assign(elementBoxes.begin(), elementBoxes.end());
    shape.selection = shape.render;
    shape.collision.clear();
    if (kind != ModelBlockShapeKind::NoCollision)
        shape.collision = shape.render;

    if (kind == ModelBlockShapeKind::Fence ||
        kind == ModelBlockShapeKind::Wall)
    {
        for (BlockBox& box : shape.collision)
            box.maximum.y = std::max(box.maximum.y, 1.5f);
    }

    // Resource models may deliberately contain duplicate elements (notably
    // multipart panes). Removing exact duplicates keeps player collision and
    // raycasting linear in the meaningful boxes only.
    const auto compact = [](std::vector<BlockBox>& boxes)
    {
        constexpr float epsilon = 0.00001f;
        const auto same = [epsilon](const BlockBox& left, const BlockBox& right)
        {
            return glm::all(glm::lessThanEqual(
                       glm::abs(left.minimum - right.minimum),
                       glm::vec3(epsilon))) &&
                   glm::all(glm::lessThanEqual(
                       glm::abs(left.maximum - right.maximum),
                       glm::vec3(epsilon)));
        };
        std::vector<BlockBox> unique;
        unique.reserve(boxes.size());
        for (const BlockBox& box : boxes)
        {
            if (std::none_of(unique.begin(), unique.end(),
                    [&box, &same](const BlockBox& candidate)
                    {
                        return same(box, candidate);
                    }))
                unique.push_back(box);
        }
        boxes = std::move(unique);
    };
    compact(shape.render);
    compact(shape.selection);
    compact(shape.collision);
    shape.refresh(occludesNeighbourFaces);
}

void clearModelBlockShapes() noexcept
{
    ModelShapes.clear();
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
