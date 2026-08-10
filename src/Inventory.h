#pragma once

#include "Item.h"

#include <array>
#include <cstdint>

class Inventory
{
public:
    static constexpr int MAIN_SLOT_COUNT = 36;
    static constexpr int ARMOR_SLOT_COUNT = 4;
    static constexpr int ARMOR_SLOT_START = MAIN_SLOT_COUNT;
    static constexpr int OFFHAND_SLOT = ARMOR_SLOT_START + ARMOR_SLOT_COUNT;
    static constexpr int SLOT_COUNT = OFFHAND_SLOT + 1;
    static constexpr int HOTBAR_SIZE = 9;
    static constexpr int MAX_STACK_SIZE = 64;

    Inventory();

    [[nodiscard]] const ItemStack& getSlot(int index) const;
    [[nodiscard]] ItemStack& getSlot(int index);

    [[nodiscard]] int getSelectedHotbarSlot() const noexcept;
    void setSelectedHotbarSlot(int slot) noexcept;
    void scrollHotbar(int direction) noexcept;

    [[nodiscard]] BlockType getSelectedBlock() const noexcept;
    [[nodiscard]] ItemType getSelectedItem() const noexcept;
    [[nodiscard]] ToolProperties getSelectedToolProperties() const noexcept;

    bool addBlock(BlockType block, int count = 1);
    bool addItem(ItemType item, int count = 1);
    bool addStack(ItemStack& stack);
    void swapWithCursor(int slot, ItemStack& cursorStack);
    [[nodiscard]] const std::array<ItemStack, SLOT_COUNT>& getSlots() const noexcept;
    void restore(
        const std::array<ItemStack, SLOT_COUNT>& slots,
        int selectedHotbarSlot
    ) noexcept;

private:
    std::array<ItemStack, SLOT_COUNT> slots_{};
    int selectedHotbarSlot_ = 0;
};
