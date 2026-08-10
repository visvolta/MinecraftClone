#pragma once

#include "Block.h"
#include "BlockEntityBase.h"
#include "Furnace.h"

#include <array>
#include <cstddef>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

struct BlockPosition
{
    int x = 0;
    int y = 0;
    int z = 0;

    [[nodiscard]] constexpr bool operator==(
        const BlockPosition&) const noexcept = default;
};

struct BlockPositionHash
{
    [[nodiscard]] std::size_t operator()(
        const BlockPosition& position) const noexcept;
};

class ChestBlockEntity final : public BlockEntity
{
public:
    static constexpr std::size_t SLOT_COUNT = 27;

    [[nodiscard]] ItemStack& getSlot(std::size_t slot);
    [[nodiscard]] const ItemStack& getSlot(std::size_t slot) const;
    [[nodiscard]] std::array<ItemStack, SLOT_COUNT>& getSlots() noexcept;
    [[nodiscard]] const std::array<ItemStack, SLOT_COUNT>& getSlots() const noexcept;
    [[nodiscard]] const mc::core::ResourceLocation& typeId() const noexcept override;
    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override;
    [[nodiscard]] BlockEntityPersistentData savePersistentData() const override;
    void loadPersistentData(const BlockEntityPersistentData& data) override;
    [[nodiscard]] std::span<ItemStack> containerItems() noexcept override;
    [[nodiscard]] std::span<const ItemStack> containerItems() const noexcept override;

private:
    std::array<ItemStack, SLOT_COUNT> slots_{};
};

struct BlockEntityRecord
{
    BlockPosition position;
    mc::core::ResourceLocation type;
    BlockEntityPersistentData data;
};

class BlockEntityTypeRegistry
{
public:
    using Factory = std::function<std::unique_ptr<BlockEntity>()>;

    void registerType(mc::core::ResourceLocation type, Factory factory);
    void freeze() noexcept;
    [[nodiscard]] bool frozen() const noexcept;
    [[nodiscard]] std::unique_ptr<BlockEntity> create(
        const mc::core::ResourceLocation& type
    ) const;
    [[nodiscard]] std::size_t size() const noexcept;

private:
    std::unordered_map<
        mc::core::ResourceLocation,
        Factory,
        mc::core::ResourceLocationHash
    > factories_;
    bool frozen_ = false;
};

[[nodiscard]] BlockEntityTypeRegistry createMinecraftBlockEntityTypes();

class BlockEntityStore
{
public:
    explicit BlockEntityStore(
        BlockEntityTypeRegistry types = createMinecraftBlockEntityTypes()
    );

    void onBlockChanged(
        const BlockPosition& position,
        BlockType oldBlock,
        BlockType newBlock
    );

    [[nodiscard]] FurnaceBlockEntity* getFurnace(
        const BlockPosition& position
    ) noexcept;
    [[nodiscard]] const FurnaceBlockEntity* getFurnace(
        const BlockPosition& position
    ) const noexcept;
    [[nodiscard]] ChestBlockEntity* getChest(
        const BlockPosition& position
    ) noexcept;
    [[nodiscard]] const ChestBlockEntity* getChest(
        const BlockPosition& position
    ) const noexcept;

    [[nodiscard]] FurnaceBlockEntity& getOrCreateFurnace(
        const BlockPosition& position
    );
    [[nodiscard]] ChestBlockEntity& getOrCreateChest(
        const BlockPosition& position
    );

    [[nodiscard]] std::vector<ItemStack> copyContainerContents(
        const BlockPosition& position
    ) const;
    bool erase(const BlockPosition& position) noexcept;
    void clear() noexcept;

    [[nodiscard]] const std::unordered_map<
        BlockPosition,
        std::unique_ptr<BlockEntity>,
        BlockPositionHash
    >& entries() const noexcept;
    [[nodiscard]] std::unordered_map<
        BlockPosition,
        std::unique_ptr<BlockEntity>,
        BlockPositionHash
    >& entries() noexcept;
    [[nodiscard]] const BlockEntityTypeRegistry& types() const noexcept;

    [[nodiscard]] std::vector<BlockEntityRecord> snapshot() const;
    void restore(std::vector<BlockEntityRecord> records);

private:
    std::unordered_map<
        BlockPosition,
        std::unique_ptr<BlockEntity>,
        BlockPositionHash
    > entries_;
    BlockEntityTypeRegistry types_;
};
