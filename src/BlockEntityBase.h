#pragma once

#include "Item.h"
#include "core/ResourceLocation.h"

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

struct BlockEntityPersistentData
{
    std::uint32_t version = 1;
    std::vector<ItemStack> items;
    std::vector<std::pair<std::string, std::int64_t>> integers;
};

class BlockEntity
{
public:
    virtual ~BlockEntity() = default;

    [[nodiscard]] virtual const mc::core::ResourceLocation& typeId() const noexcept = 0;
    [[nodiscard]] virtual std::unique_ptr<BlockEntity> clone() const = 0;
    [[nodiscard]] virtual BlockEntityPersistentData savePersistentData() const = 0;
    virtual void loadPersistentData(const BlockEntityPersistentData& data) = 0;

    [[nodiscard]] virtual std::span<ItemStack> containerItems() noexcept
    {
        return {};
    }
    [[nodiscard]] virtual std::span<const ItemStack> containerItems() const noexcept
    {
        return {};
    }
};
