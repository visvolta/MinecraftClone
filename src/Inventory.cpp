#include "Inventory.h"

#include <algorithm>
#include <stdexcept>

Inventory::Inventory() = default;

const ItemStack& Inventory::getSlot(int index) const
{
    if (index < 0 || index >= SLOT_COUNT)
        throw std::out_of_range("Inventory slot outside valid range");

    return slots_[static_cast<std::size_t>(index)];
}

ItemStack& Inventory::getSlot(int index)
{
    if (index < 0 || index >= SLOT_COUNT)
        throw std::out_of_range("Inventory slot outside valid range");

    return slots_[static_cast<std::size_t>(index)];
}

int Inventory::getSelectedHotbarSlot() const noexcept
{
    return selectedHotbarSlot_;
}

void Inventory::setSelectedHotbarSlot(int slot) noexcept
{
    selectedHotbarSlot_ = std::clamp(slot, 0, HOTBAR_SIZE - 1);
}

void Inventory::scrollHotbar(int direction) noexcept
{
    selectedHotbarSlot_ =
        (selectedHotbarSlot_ + direction + HOTBAR_SIZE) % HOTBAR_SIZE;
}

BlockType Inventory::getSelectedBlock() const noexcept
{
    const ItemStack& stack =
        slots_[static_cast<std::size_t>(selectedHotbarSlot_)];

    return stack.empty() ? BlockType::Air : blockFromItem(stack.item);
}

ItemType Inventory::getSelectedItem() const noexcept
{
    const ItemStack& stack =
        slots_[static_cast<std::size_t>(selectedHotbarSlot_)];
    return stack.empty() ? ItemType::Empty : stack.item;
}

ToolProperties Inventory::getSelectedToolProperties() const noexcept
{
    return getItemToolProperties(getSelectedItem());
}

bool Inventory::addBlock(BlockType block, int count)
{
    if (block == BlockType::Air || count <= 0)
        return false;

    ItemStack stack{
        block,
        static_cast<std::uint8_t>(
            std::min(count, MAX_STACK_SIZE)
        )
    };

    return addStack(stack);
}

bool Inventory::addItem(ItemType item, int count)
{
    if (item == ItemType::Empty || count <= 0)
        return false;

    ItemStack stack{
        item,
        static_cast<std::uint8_t>(
            std::min<int>(
                count,
                getItemProperties(item).maximumStackSize
            )
        )
    };
    return addStack(stack);
}

bool Inventory::addStack(ItemStack& stack)
{
    if (stack.empty())
        return true;

    for (int index = 0; index < MAIN_SLOT_COUNT; ++index)
    {
        ItemStack& slot = slots_[static_cast<std::size_t>(index)];
        if (!slot.canStackWith(stack) ||
            slot.count >= slot.maximumStackSize())
        {
            continue;
        }

        const int room = slot.maximumStackSize() - slot.count;
        const int moved = std::min<int>(room, stack.count);
        slot.count = static_cast<std::uint8_t>(slot.count + moved);
        stack.count = static_cast<std::uint8_t>(stack.count - moved);

        if (stack.count == 0)
        {
            stack.clear();
            return true;
        }
    }

    for (int index = 0; index < MAIN_SLOT_COUNT; ++index)
    {
        ItemStack& slot = slots_[static_cast<std::size_t>(index)];
        if (!slot.empty())
            continue;

        const int moved = std::min<int>(
            stack.maximumStackSize(), stack.count
        );

        slot.item = stack.item;
        slot.count = static_cast<std::uint8_t>(moved);
        slot.damage = stack.damage;
        stack.count = static_cast<std::uint8_t>(stack.count - moved);

        if (stack.count == 0)
        {
            stack.clear();
            return true;
        }
    }

    return false;
}

void Inventory::swapWithCursor(
    int slot,
    ItemStack& cursorStack)
{
    ItemStack& inventoryStack = getSlot(slot);
    std::swap(inventoryStack, cursorStack);
}

const std::array<ItemStack, Inventory::SLOT_COUNT>&
Inventory::getSlots() const noexcept
{
    return slots_;
}

void Inventory::restore(
    const std::array<ItemStack, SLOT_COUNT>& slots,
    int selectedHotbarSlot) noexcept
{
    slots_ = slots;
    setSelectedHotbarSlot(selectedHotbarSlot);
}
