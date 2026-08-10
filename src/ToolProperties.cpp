#include "ToolProperties.h"

#include <array>
#include <cstddef>

namespace
{
struct MaterialProperties
{
    float miningSpeed;
    int harvestLevel;
};

constexpr std::array<MaterialProperties, 6> materialProperties = {{
    {1.0f, -1}, // None / hand
    {2.0f, 0},  // Wood
    {4.0f, 1},  // Stone
    {6.0f, 2},  // Iron
    {8.0f, 3},  // Diamond
    {12.0f, 0}  // Gold
}};
}

ToolProperties makeToolProperties(
    ToolType type,
    ToolMaterial material) noexcept
{
    if (type == ToolType::None)
        return getHandToolProperties();

    const MaterialProperties& values =
        materialProperties[static_cast<std::size_t>(material)];

    return {
        type,
        material,
        values.miningSpeed,
        values.harvestLevel
    };
}
