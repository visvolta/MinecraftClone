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
// so the wider property payload does not increase per-voxel storage. Legacy
// blocks continue to use their low four metadata bits; resource-defined blocks
// use the full state index required by 1.12 multipart models.
class BlockState
{
public:
    constexpr BlockState() noexcept = default;

    constexpr explicit BlockState(
        BlockType block,
        std::uint16_t properties = 0) noexcept
        : blockRuntimeId_(static_cast<core::RuntimeId>(block)),
          properties_(properties)
    {
    }

    [[nodiscard]] static constexpr BlockState fromRuntimeId(
        core::RuntimeId blockRuntimeId,
        std::uint16_t properties = 0) noexcept
    {
        return BlockState(blockRuntimeId, properties, RuntimeIdTag{});
    }

    [[nodiscard]] constexpr core::RuntimeId blockRuntimeId() const noexcept
    {
        return blockRuntimeId_;
    }

    [[nodiscard]] constexpr BlockType block() const noexcept
    {
        return blockRuntimeId_ <= static_cast<core::RuntimeId>(BlockType::Cobweb)
            ? static_cast<BlockType>(blockRuntimeId_)
            : BlockType::Air;
    }

    [[nodiscard]] constexpr std::uint16_t properties() const noexcept
    {
        return properties_;
    }

    [[nodiscard]] constexpr BlockState withProperties(
        std::uint16_t properties) const noexcept
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

    constexpr BlockState(
        core::RuntimeId blockRuntimeId,
        std::uint16_t properties,
        RuntimeIdTag) noexcept
        : blockRuntimeId_(blockRuntimeId),
          properties_(properties)
    {
    }

    core::RuntimeId blockRuntimeId_ = 0;
    std::uint16_t properties_ = 0;
};

static_assert(std::is_trivially_copyable_v<BlockState>);

struct BlockStateHash
{
    [[nodiscard]] constexpr std::size_t operator()(
        BlockState state) const noexcept
    {
        return (static_cast<std::size_t>(state.blockRuntimeId()) << 16U) |
               state.properties();
    }
};
}
