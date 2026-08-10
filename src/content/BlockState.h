#pragma once

#include "Block.h"
#include "core/Registry.h"

#include <compare>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace mc::content
{
// Immutable registry-backed voxel value. Chunks store compact palette indices,
// so adding a 32-bit global registry ID here does not increase per-voxel
// storage. The low four property bits preserve Release 1.12 block states.
class BlockState
{
public:
    constexpr BlockState() noexcept = default;

    constexpr explicit BlockState(
        BlockType block,
        std::uint8_t properties = 0) noexcept
        : blockRuntimeId_(static_cast<core::RuntimeId>(block)),
          properties_(properties & PropertyMask)
    {
    }

    [[nodiscard]] static constexpr BlockState fromRuntimeId(
        core::RuntimeId blockRuntimeId,
        std::uint8_t properties = 0) noexcept
    {
        return BlockState(blockRuntimeId, properties, RuntimeIdTag{});
    }

    [[nodiscard]] constexpr core::RuntimeId blockRuntimeId() const noexcept
    {
        return blockRuntimeId_;
    }

    [[nodiscard]] constexpr BlockType block() const noexcept
    {
        return blockRuntimeId_ <= static_cast<core::RuntimeId>(BlockType::TNT)
            ? static_cast<BlockType>(blockRuntimeId_)
            : BlockType::Air;
    }

    [[nodiscard]] constexpr std::uint8_t properties() const noexcept
    {
        return properties_;
    }

    [[nodiscard]] constexpr BlockState withProperties(
        std::uint8_t properties) const noexcept
    {
        return fromRuntimeId(blockRuntimeId_, properties);
    }

    [[nodiscard]] constexpr bool isAir() const noexcept
    {
        return blockRuntimeId_ == static_cast<core::RuntimeId>(BlockType::Air);
    }

    [[nodiscard]] constexpr auto operator<=>(const BlockState&) const = default;

private:
    struct RuntimeIdTag {};

    static constexpr std::uint8_t PropertyMask = 0x0FU;

    constexpr BlockState(
        core::RuntimeId blockRuntimeId,
        std::uint8_t properties,
        RuntimeIdTag) noexcept
        : blockRuntimeId_(blockRuntimeId),
          properties_(properties & PropertyMask)
    {
    }

    core::RuntimeId blockRuntimeId_ = 0;
    std::uint8_t properties_ = 0;
};

static_assert(std::is_trivially_copyable_v<BlockState>);

struct BlockStateHash
{
    [[nodiscard]] constexpr std::size_t operator()(
        BlockState state) const noexcept
    {
        return (static_cast<std::size_t>(state.blockRuntimeId()) << 4U) |
               state.properties();
    }
};
}
