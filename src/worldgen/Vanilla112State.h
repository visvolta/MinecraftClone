#pragma once

#include "content/BlockState.h"
#include "core/ResourceLocation.h"

#include <initializer_list>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mc112
{
using Property = std::pair<std::string, std::string>;

[[nodiscard]] mc::content::BlockState state(
    std::string_view name,
    std::initializer_list<Property> properties = {});
[[nodiscard]] mc::content::BlockState state(
    std::string_view name,
    std::span<const Property> properties);

[[nodiscard]] std::optional<mc::content::BlockState> tryState(
    std::string_view name,
    std::initializer_list<Property> properties = {});
[[nodiscard]] std::optional<mc::content::BlockState> tryState(
    std::string_view name,
    std::span<const Property> properties);

// Resolve a palette entry from the 1.12 registry/state schema into the
// clone's resource-backed, variant-split catalog. The source name and
// properties are the exact values stored by 1.12 structure-template NBT.
[[nodiscard]] std::optional<mc::content::BlockState> tryVanilla112State(
    std::string_view registryName,
    std::span<const Property> properties);
[[nodiscard]] mc::content::BlockState vanilla112State(
    std::string_view registryName,
    std::span<const Property> properties);

[[nodiscard]] bool named(mc::content::BlockState state, std::string_view name) noexcept;
[[nodiscard]] bool isAir(mc::content::BlockState state) noexcept;
[[nodiscard]] bool isLiquid(mc::content::BlockState state) noexcept;
[[nodiscard]] bool isWater(mc::content::BlockState state) noexcept;
[[nodiscard]] bool isLava(mc::content::BlockState state) noexcept;
[[nodiscard]] bool isLeaf(mc::content::BlockState state) noexcept;
[[nodiscard]] bool isLog(mc::content::BlockState state) noexcept;
[[nodiscard]] bool isSolid(mc::content::BlockState state) noexcept;
[[nodiscard]] bool isOpaque(mc::content::BlockState state) noexcept;
[[nodiscard]] bool isPlant(mc::content::BlockState state) noexcept;
[[nodiscard]] bool isReplaceableByStructure(mc::content::BlockState state) noexcept;
[[nodiscard]] bool isNaturalStone(mc::content::BlockState state) noexcept;
[[nodiscard]] bool isDirtLike(mc::content::BlockState state) noexcept;
[[nodiscard]] std::string_view path(mc::content::BlockState state) noexcept;
}
