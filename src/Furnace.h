#pragma once

#include "BlockEntityBase.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <unordered_map>

namespace mc::content { class ContentCatalog; }

struct FurnacePersistentState
{
    std::array<ItemStack, 3> slots{};
    int burnTime = 0;
    int currentFuelBurnTime = 0;
    int cookTime = 0;
};

class FurnaceBlockEntity final : public BlockEntity
{
public:
    static constexpr int COOK_TIME_TICKS = 200;

    enum Slot : std::size_t
    {
        Input = 0,
        Fuel = 1,
        Output = 2
    };

    [[nodiscard]] ItemStack& getSlot(Slot slot) noexcept;
    [[nodiscard]] const ItemStack& getSlot(Slot slot) const noexcept;
    [[nodiscard]] const std::array<ItemStack, 3>& getSlots() const noexcept;
    [[nodiscard]] const mc::core::ResourceLocation& typeId() const noexcept override;
    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override;
    [[nodiscard]] BlockEntityPersistentData savePersistentData() const override;
    void loadPersistentData(const BlockEntityPersistentData& data) override;
    [[nodiscard]] std::span<ItemStack> containerItems() noexcept override;
    [[nodiscard]] std::span<const ItemStack> containerItems() const noexcept override;

    // Returns true when the lit/unlit block state must change.
    [[nodiscard]] bool tick() noexcept;
    [[nodiscard]] bool isBurning() const noexcept;
    [[nodiscard]] int getCookProgressScaled(int pixels) const noexcept;
    [[nodiscard]] int getBurnTimeScaled(int pixels) const noexcept;
    [[nodiscard]] FurnacePersistentState persistentState() const noexcept;
    void restorePersistentState(const FurnacePersistentState& state) noexcept;

private:
    std::array<ItemStack, 3> slots_{};
    int burnTime_ = 0;
    int currentFuelBurnTime_ = 0;
    int cookTime_ = 0;

    [[nodiscard]] bool canSmelt() const noexcept;
    void smeltItem() noexcept;
};

class FurnaceRecipeRegistry
{
public:
    FurnaceRecipeRegistry(
        const std::filesystem::path& assetRoot,
        const mc::content::ContentCatalog& content
    );

    [[nodiscard]] ItemStack smeltingResult(ItemType input) const noexcept;
    [[nodiscard]] int fuelBurnTime(ItemType fuel) const noexcept;
    [[nodiscard]] std::size_t smeltingRecipeCount() const noexcept;
    [[nodiscard]] std::size_t fuelCount() const noexcept;

    static void initialize(
        const std::filesystem::path& assetRoot,
        const mc::content::ContentCatalog& content
    );
    [[nodiscard]] static const FurnaceRecipeRegistry* active() noexcept;

private:
    std::unordered_map<ItemType, ItemStack> smeltingRecipes_;
    std::unordered_map<ItemType, int> fuels_;
};

[[nodiscard]] ItemStack getSmeltingResult(ItemType input) noexcept;
[[nodiscard]] int getFuelBurnTime(ItemType fuel) noexcept;
