#pragma once

enum class ToolType
{
    None,
    Pickaxe,
    Axe,
    Shovel
};

enum class ToolMaterial
{
    None,
    Wood,
    Stone,
    Iron,
    Diamond,
    Gold
};

struct ToolProperties
{
    ToolType type = ToolType::None;
    ToolMaterial material = ToolMaterial::None;
    float miningSpeed = 1.0f;
    int harvestLevel = -1;
    int efficiencyLevel = 0;
};

// Tools are not inventory items yet. These profiles keep Beta's tool values in
// one place so an item system can select a profile later without changing the
// block-breaking formula.
[[nodiscard]] ToolProperties makeToolProperties(
    ToolType type,
    ToolMaterial material) noexcept;

[[nodiscard]] constexpr ToolProperties getHandToolProperties() noexcept
{
    return {};
}
