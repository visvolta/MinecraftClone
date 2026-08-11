#include "content/BlockStateLogic.h"

#include "BlockShape.h"
#include "content/ContentCatalog.h"

#include <algorithm>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mc::content
{
namespace
{
bool endsWith(std::string_view value, std::string_view suffix) noexcept
{
    return value.size() >= suffix.size() &&
           value.substr(value.size() - suffix.size()) == suffix;
}

enum class ConnectedKind { None, Fence, Wall, Pane, Stairs };

ConnectedKind connectedKind(std::string_view name) noexcept
{
    if (name == "fence" || endsWith(name, "_fence"))
        return ConnectedKind::Fence;
    if (endsWith(name, "_wall"))
        return ConnectedKind::Wall;
    if (name == "glass_pane" || name == "iron_bars" ||
        endsWith(name, "_stained_glass_pane"))
        return ConnectedKind::Pane;
    if (endsWith(name, "_stairs"))
        return ConnectedKind::Stairs;
    return ConnectedKind::None;
}

std::string propertyValue(
    const ContentCatalog& catalog,
    BlockState state,
    std::string_view property)
{
    for (const auto& [name, value] : catalog.serializeStateProperties(state))
        if (name == property)
            return value;
    return {};
}

BlockState withProperties(
    const ContentCatalog& catalog,
    BlockState state,
    std::initializer_list<std::pair<std::string_view, std::string_view>> changes)
{
    const core::ResourceLocation* name = catalog.blockName(state);
    if (name == nullptr)
        return state;
    std::vector<std::pair<std::string, std::string>> properties =
        catalog.serializeStateProperties(state);
    for (const auto& [changedName, changedValue] : changes)
    {
        const auto found = std::find_if(
            properties.begin(), properties.end(),
            [changedName](const auto& candidate)
            {
                return candidate.first == changedName;
            }
        );
        if (found != properties.end())
            found->second = changedValue;
    }
    const auto resolved = catalog.state(*name, properties);
    return resolved.value_or(state);
}

bool isFullSolid(BlockState state) noexcept
{
    if (state.isAir())
        return false;
    constexpr float epsilon = 0.00001f;
    for (const BlockBox& box : getBlockShape(state).collisionBoxes)
    {
        if (box.minimum.x <= epsilon && box.minimum.y <= epsilon &&
            box.minimum.z <= epsilon && box.maximum.x >= 1.0f - epsilon &&
            box.maximum.y >= 1.0f - epsilon &&
            box.maximum.z >= 1.0f - epsilon)
            return true;
    }
    return false;
}

bool connects(
    const ContentCatalog& catalog,
    ConnectedKind kind,
    BlockState neighbour) noexcept
{
    const core::ResourceLocation* neighbourName = catalog.blockName(neighbour);
    const ConnectedKind neighbourKind = neighbourName == nullptr
        ? ConnectedKind::None
        : connectedKind(neighbourName->path());
    const std::string_view path = neighbourName == nullptr
        ? std::string_view{} : std::string_view(neighbourName->path());
    const bool commonException = path == "barrier" || path == "beacon" ||
        path == "cauldron" || path == "glowstone" || path == "ice" ||
        path == "sea_lantern" || path == "piston" ||
        path == "sticky_piston" || path == "piston_head" ||
        path == "melon_block" || path == "pumpkin" ||
        path == "lit_pumpkin" || path == "leaves" || path == "leaves2" ||
        endsWith(path, "_leaves") || endsWith(path, "_shulker_box");
    switch (kind)
    {
        case ConnectedKind::Fence:
            return neighbourKind == ConnectedKind::Fence ||
                   (neighbourName != nullptr &&
                    endsWith(neighbourName->path(), "_fence_gate")) ||
                   (!commonException && path != "glass" &&
                    path != "stained_glass" &&
                    !endsWith(path, "_trapdoor") && isFullSolid(neighbour));
        case ConnectedKind::Wall:
            return neighbourKind == ConnectedKind::Wall ||
                   (neighbourName != nullptr &&
                    endsWith(neighbourName->path(), "_fence_gate")) ||
                   (!commonException && path != "glass" &&
                    path != "stained_glass" &&
                    !endsWith(path, "_trapdoor") && isFullSolid(neighbour));
        case ConnectedKind::Pane:
            return neighbourKind == ConnectedKind::Pane ||
                   (!commonException && isFullSolid(neighbour));
        default:
            return false;
    }
}

int directionIndex(std::string_view facing) noexcept
{
    if (facing == "north") return 0;
    if (facing == "east") return 1;
    if (facing == "south") return 2;
    if (facing == "west") return 3;
    return -1;
}

bool differentStairs(
    const ContentCatalog& catalog,
    BlockState current,
    BlockState neighbour)
{
    const core::ResourceLocation* name = catalog.blockName(neighbour);
    return name == nullptr || connectedKind(name->path()) != ConnectedKind::Stairs ||
           propertyValue(catalog, neighbour, "facing") !=
               propertyValue(catalog, current, "facing") ||
           propertyValue(catalog, neighbour, "half") !=
               propertyValue(catalog, current, "half");
}

BlockState resolveStairShape(
    const ContentCatalog& catalog,
    BlockState state,
    const std::array<BlockState, 4>& neighbours)
{
    const std::string facing = propertyValue(catalog, state, "facing");
    const std::string half = propertyValue(catalog, state, "half");
    const int direction = directionIndex(facing);
    if (direction < 0 || half.empty())
        return state;

    const BlockState front = neighbours[static_cast<std::size_t>(direction)];
    const core::ResourceLocation* frontName = catalog.blockName(front);
    if (frontName != nullptr &&
        connectedKind(frontName->path()) == ConnectedKind::Stairs &&
        propertyValue(catalog, front, "half") == half)
    {
        const int frontDirection = directionIndex(
            propertyValue(catalog, front, "facing")
        );
        if (frontDirection >= 0 && (frontDirection & 1) != (direction & 1) &&
            differentStairs(
                catalog, state,
                neighbours[static_cast<std::size_t>((frontDirection + 2) & 3)]
            ))
        {
            return withProperties(catalog, state, {{
                "shape",
                frontDirection == ((direction + 3) & 3)
                    ? "outer_left" : "outer_right"
            }});
        }
    }

    const BlockState back = neighbours[static_cast<std::size_t>((direction + 2) & 3)];
    const core::ResourceLocation* backName = catalog.blockName(back);
    if (backName != nullptr &&
        connectedKind(backName->path()) == ConnectedKind::Stairs &&
        propertyValue(catalog, back, "half") == half)
    {
        const int backDirection = directionIndex(
            propertyValue(catalog, back, "facing")
        );
        if (backDirection >= 0 && (backDirection & 1) != (direction & 1) &&
            differentStairs(
                catalog, state,
                neighbours[static_cast<std::size_t>(backDirection)]
            ))
        {
            return withProperties(catalog, state, {{
                "shape",
                backDirection == ((direction + 3) & 3)
                    ? "inner_left" : "inner_right"
            }});
        }
    }
    return withProperties(catalog, state, {{"shape", "straight"}});
}
}

BlockState resolveActualBlockState(
    BlockState state,
    const std::array<BlockState, 4>& horizontalNeighbours,
    BlockState above)
{
    const ContentCatalog* catalog = ContentCatalog::active();
    const core::ResourceLocation* name = catalog == nullptr
        ? nullptr : catalog->blockName(state);
    if (catalog == nullptr || name == nullptr)
        return state;
    const ConnectedKind kind = connectedKind(name->path());
    if (kind == ConnectedKind::Stairs)
        return resolveStairShape(*catalog, state, horizontalNeighbours);
    if (kind == ConnectedKind::None)
        return state;

    const bool north = connects(*catalog, kind, horizontalNeighbours[0]);
    const bool east = connects(*catalog, kind, horizontalNeighbours[1]);
    const bool south = connects(*catalog, kind, horizontalNeighbours[2]);
    const bool west = connects(*catalog, kind, horizontalNeighbours[3]);
    if (kind == ConnectedKind::Wall)
    {
        const bool straight = (north && south && !east && !west) ||
                              (!north && !south && east && west);
        return withProperties(*catalog, state, {
            {"up", !straight || !above.isAir() ? "true" : "false"},
            {"north", north ? "true" : "false"},
            {"east", east ? "true" : "false"},
            {"south", south ? "true" : "false"},
            {"west", west ? "true" : "false"}
        });
    }
    return withProperties(*catalog, state, {
        {"north", north ? "true" : "false"},
        {"east", east ? "true" : "false"},
        {"south", south ? "true" : "false"},
        {"west", west ? "true" : "false"}
    });
}
}
