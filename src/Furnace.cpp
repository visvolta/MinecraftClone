#include "Furnace.h"

#include "content/ContentCatalog.h"

#include <algorithm>
#include <fstream>
#include <limits>
#include <memory>
#include <string_view>

#include <nlohmann/json.hpp>

namespace
{
using Json = nlohmann::json;
std::unique_ptr<FurnaceRecipeRegistry> ActiveFurnaceRecipes;

Json readDataFile(const std::filesystem::path& path)
{
    std::ifstream input(path);
    if (!input)
        throw std::runtime_error("Could not open furnace data: " + path.string());
    try
    {
        return Json::parse(input);
    }
    catch (const Json::exception& error)
    {
        throw std::runtime_error(
            "Invalid furnace data " + path.string() + ": " + error.what()
        );
    }
}

ItemType resolveItem(
    std::string_view name,
    const mc::content::ContentCatalog& content)
{
    const auto item = content.legacyItem(mc::core::ResourceLocation(name));
    if (!item)
        throw std::runtime_error("Furnace data references an unknown item");
    return *item;
}
}

ItemStack getSmeltingResult(ItemType input) noexcept
{
    const FurnaceRecipeRegistry* recipes = FurnaceRecipeRegistry::active();
    return recipes == nullptr ? ItemStack{} : recipes->smeltingResult(input);
}

int getFuelBurnTime(ItemType fuel) noexcept
{
    const FurnaceRecipeRegistry* recipes = FurnaceRecipeRegistry::active();
    return recipes == nullptr ? 0 : recipes->fuelBurnTime(fuel);
}

FurnaceRecipeRegistry::FurnaceRecipeRegistry(
    const std::filesystem::path& assetRoot,
    const mc::content::ContentCatalog& content)
{
    const Json smelting = readDataFile(
        assetRoot / "minecraft" / "smelting.json"
    );
    for (const Json& recipe : smelting.at("recipes"))
    {
        const ItemType input = resolveItem(
            recipe.at("input").get<std::string>(), content
        );
        const Json& resultJson = recipe.at("result");
        const ItemType resultItem = resolveItem(
            resultJson.at("item").get<std::string>(), content
        );
        const int count = resultJson.value("count", 1);
        if (count < 1 || count > 255)
            throw std::runtime_error("Smelting recipe has an invalid count");
        const ItemStack result{
            resultItem,
            static_cast<std::uint8_t>(count)
        };
        if (!smeltingRecipes_.emplace(input, result).second)
            throw std::runtime_error("Duplicate smelting recipe input");
    }

    const Json fuels = readDataFile(assetRoot / "minecraft" / "fuels.json");
    for (const Json& fuelGroup : fuels.at("fuels"))
    {
        const int burnTime = fuelGroup.at("burn_time").get<int>();
        if (burnTime <= 0)
            throw std::runtime_error("Fuel must have a positive burn time");
        for (const Json& item : fuelGroup.at("items"))
        {
            const ItemType type = resolveItem(item.get<std::string>(), content);
            if (!fuels_.emplace(type, burnTime).second)
                throw std::runtime_error("Duplicate furnace fuel");
        }
    }
}

ItemStack FurnaceRecipeRegistry::smeltingResult(ItemType input) const noexcept
{
    const auto recipe = smeltingRecipes_.find(input);
    return recipe == smeltingRecipes_.end() ? ItemStack{} : recipe->second;
}

int FurnaceRecipeRegistry::fuelBurnTime(ItemType fuel) const noexcept
{
    const auto entry = fuels_.find(fuel);
    return entry == fuels_.end() ? 0 : entry->second;
}

std::size_t FurnaceRecipeRegistry::smeltingRecipeCount() const noexcept
{
    return smeltingRecipes_.size();
}

std::size_t FurnaceRecipeRegistry::fuelCount() const noexcept
{
    return fuels_.size();
}

void FurnaceRecipeRegistry::initialize(
    const std::filesystem::path& assetRoot,
    const mc::content::ContentCatalog& content)
{
    ActiveFurnaceRecipes = std::make_unique<FurnaceRecipeRegistry>(
        assetRoot, content
    );
}

const FurnaceRecipeRegistry* FurnaceRecipeRegistry::active() noexcept
{
    return ActiveFurnaceRecipes.get();
}

ItemStack& FurnaceBlockEntity::getSlot(Slot slot) noexcept
{
    return slots_[static_cast<std::size_t>(slot)];
}

const ItemStack& FurnaceBlockEntity::getSlot(Slot slot) const noexcept
{
    return slots_[static_cast<std::size_t>(slot)];
}

const std::array<ItemStack, 3>& FurnaceBlockEntity::getSlots() const noexcept
{
    return slots_;
}

const mc::core::ResourceLocation& FurnaceBlockEntity::typeId() const noexcept
{
    static const mc::core::ResourceLocation id("minecraft:furnace");
    return id;
}

std::unique_ptr<BlockEntity> FurnaceBlockEntity::clone() const
{
    return std::make_unique<FurnaceBlockEntity>(*this);
}

BlockEntityPersistentData FurnaceBlockEntity::savePersistentData() const
{
    BlockEntityPersistentData data;
    data.items.assign(slots_.begin(), slots_.end());
    data.integers = {
        {"burn_time", burnTime_},
        {"current_fuel_burn_time", currentFuelBurnTime_},
        {"cook_time", cookTime_}
    };
    return data;
}

void FurnaceBlockEntity::loadPersistentData(
    const BlockEntityPersistentData& data)
{
    FurnacePersistentState state;
    const auto persistedInt = [](std::int64_t value) noexcept
    {
        return static_cast<int>(std::clamp<std::int64_t>(
            value,
            0,
            std::numeric_limits<int>::max()
        ));
    };
    const std::size_t count = std::min(data.items.size(), state.slots.size());
    std::copy_n(data.items.begin(), count, state.slots.begin());
    for (const auto& [name, value] : data.integers)
    {
        if (name == "burn_time")
            state.burnTime = persistedInt(value);
        else if (name == "current_fuel_burn_time")
            state.currentFuelBurnTime = persistedInt(value);
        else if (name == "cook_time")
            state.cookTime = persistedInt(value);
    }
    restorePersistentState(state);
}

std::span<ItemStack> FurnaceBlockEntity::containerItems() noexcept
{
    return slots_;
}

std::span<const ItemStack> FurnaceBlockEntity::containerItems() const noexcept
{
    return slots_;
}

bool FurnaceBlockEntity::tick() noexcept
{
    const bool wasBurning = isBurning();
    if (burnTime_ > 0)
        --burnTime_;

    if (burnTime_ == 0 && canSmelt())
    {
        ItemStack& fuel = getSlot(Fuel);
        currentFuelBurnTime_ = getFuelBurnTime(fuel.item);
        burnTime_ = currentFuelBurnTime_;
        if (burnTime_ > 0)
        {
            --fuel.count;
            if (fuel.count == 0)
                fuel.clear();
        }
    }

    if (isBurning() && canSmelt())
    {
        ++cookTime_;
        if (cookTime_ >= COOK_TIME_TICKS)
        {
            cookTime_ = 0;
            smeltItem();
        }
    }
    else
    {
        cookTime_ = 0;
    }

    return wasBurning != isBurning();
}

bool FurnaceBlockEntity::isBurning() const noexcept
{
    return burnTime_ > 0;
}

int FurnaceBlockEntity::getCookProgressScaled(int pixels) const noexcept
{
    return std::clamp(cookTime_ * pixels / COOK_TIME_TICKS, 0, pixels);
}

int FurnaceBlockEntity::getBurnTimeScaled(int pixels) const noexcept
{
    const int total = currentFuelBurnTime_ == 0 ? COOK_TIME_TICKS : currentFuelBurnTime_;
    return std::clamp(burnTime_ * pixels / total, 0, pixels);
}

FurnacePersistentState FurnaceBlockEntity::persistentState() const noexcept
{
    return {
        slots_, burnTime_, currentFuelBurnTime_, cookTime_
    };
}

void FurnaceBlockEntity::restorePersistentState(
    const FurnacePersistentState& state) noexcept
{
    slots_ = state.slots;
    burnTime_ = std::max(0, state.burnTime);
    currentFuelBurnTime_ = std::max(0, state.currentFuelBurnTime);
    cookTime_ = std::clamp(state.cookTime, 0, COOK_TIME_TICKS - 1);
}

bool FurnaceBlockEntity::canSmelt() const noexcept
{
    const ItemStack& input = getSlot(Input);
    if (input.empty())
        return false;

    const ItemStack result = getSmeltingResult(input.item);
    if (result.empty())
        return false;

    const ItemStack& output = getSlot(Output);
    if (output.empty())
        return true;
    if (!output.canStackWith(result))
        return false;
    return static_cast<int>(output.count) + result.count <=
           output.maximumStackSize();
}

void FurnaceBlockEntity::smeltItem() noexcept
{
    if (!canSmelt())
        return;

    ItemStack& input = getSlot(Input);
    ItemStack& output = getSlot(Output);
    const ItemStack result = getSmeltingResult(input.item);

    if (output.empty())
        output = result;
    else
        output.count = static_cast<std::uint8_t>(output.count + result.count);

    --input.count;
    if (input.count == 0)
        input.clear();
}
