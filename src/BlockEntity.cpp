#include "BlockEntity.h"

#include <algorithm>
#include <functional>
#include <stdexcept>
#include <utility>

namespace
{
const mc::core::ResourceLocation FurnaceType("minecraft:furnace");
const mc::core::ResourceLocation ChestType("minecraft:chest");
}

std::size_t BlockPositionHash::operator()(
    const BlockPosition& position) const noexcept
{
    std::size_t seed = std::hash<int>{}(position.x);
    seed ^= std::hash<int>{}(position.y) + 0x9e3779b9U +
            (seed << 6U) + (seed >> 2U);
    seed ^= std::hash<int>{}(position.z) + 0x9e3779b9U +
            (seed << 6U) + (seed >> 2U);
    return seed;
}

ItemStack& ChestBlockEntity::getSlot(std::size_t slot)
{
    if (slot >= slots_.size())
        throw std::out_of_range("Chest slot outside 0..26");
    return slots_[slot];
}

const ItemStack& ChestBlockEntity::getSlot(std::size_t slot) const
{
    if (slot >= slots_.size())
        throw std::out_of_range("Chest slot outside 0..26");
    return slots_[slot];
}

std::array<ItemStack, ChestBlockEntity::SLOT_COUNT>&
ChestBlockEntity::getSlots() noexcept
{
    return slots_;
}

const std::array<ItemStack, ChestBlockEntity::SLOT_COUNT>&
ChestBlockEntity::getSlots() const noexcept
{
    return slots_;
}

const mc::core::ResourceLocation& ChestBlockEntity::typeId() const noexcept
{
    return ChestType;
}

std::unique_ptr<BlockEntity> ChestBlockEntity::clone() const
{
    return std::make_unique<ChestBlockEntity>(*this);
}

BlockEntityPersistentData ChestBlockEntity::savePersistentData() const
{
    BlockEntityPersistentData data;
    data.items.assign(slots_.begin(), slots_.end());
    return data;
}

void ChestBlockEntity::loadPersistentData(
    const BlockEntityPersistentData& data)
{
    slots_.fill({});
    std::copy_n(
        data.items.begin(),
        std::min(data.items.size(), slots_.size()),
        slots_.begin()
    );
}

std::span<ItemStack> ChestBlockEntity::containerItems() noexcept
{
    return slots_;
}

std::span<const ItemStack> ChestBlockEntity::containerItems() const noexcept
{
    return slots_;
}

void BlockEntityTypeRegistry::registerType(
    mc::core::ResourceLocation type,
    Factory factory)
{
    if (frozen_)
        throw std::logic_error("Cannot modify a frozen block entity registry");
    if (!factory)
        throw std::invalid_argument("Block entity factory cannot be empty");
    if (!factories_.emplace(std::move(type), std::move(factory)).second)
        throw std::invalid_argument("Duplicate block entity type");
}

void BlockEntityTypeRegistry::freeze() noexcept
{
    frozen_ = true;
}

bool BlockEntityTypeRegistry::frozen() const noexcept
{
    return frozen_;
}

std::unique_ptr<BlockEntity> BlockEntityTypeRegistry::create(
    const mc::core::ResourceLocation& type) const
{
    const auto factory = factories_.find(type);
    return factory == factories_.end() ? nullptr : factory->second();
}

std::size_t BlockEntityTypeRegistry::size() const noexcept
{
    return factories_.size();
}

BlockEntityTypeRegistry createMinecraftBlockEntityTypes()
{
    BlockEntityTypeRegistry types;
    types.registerType(FurnaceType, []
    {
        return std::make_unique<FurnaceBlockEntity>();
    });
    types.registerType(ChestType, []
    {
        return std::make_unique<ChestBlockEntity>();
    });
    types.freeze();
    return types;
}

BlockEntityStore::BlockEntityStore(BlockEntityTypeRegistry types)
    : types_(std::move(types))
{
    if (!types_.frozen())
        throw std::invalid_argument("Block entity type registry must be frozen");
}

void BlockEntityStore::onBlockChanged(
    const BlockPosition& position,
    BlockType oldBlock,
    BlockType newBlock)
{
    if (isFurnace(oldBlock) && isFurnace(newBlock))
        return;
    if (oldBlock == BlockType::Chest && newBlock == BlockType::Chest)
        return;

    entries_.erase(position);
    const mc::core::ResourceLocation* type = nullptr;
    if (isFurnace(newBlock))
        type = &FurnaceType;
    else if (newBlock == BlockType::Chest)
        type = &ChestType;
    if (type != nullptr)
        entries_.emplace(position, types_.create(*type));
}

FurnaceBlockEntity* BlockEntityStore::getFurnace(
    const BlockPosition& position) noexcept
{
    const auto found = entries_.find(position);
    return found == entries_.end()
        ? nullptr
        : dynamic_cast<FurnaceBlockEntity*>(found->second.get());
}

const FurnaceBlockEntity* BlockEntityStore::getFurnace(
    const BlockPosition& position) const noexcept
{
    const auto found = entries_.find(position);
    return found == entries_.end()
        ? nullptr
        : dynamic_cast<const FurnaceBlockEntity*>(found->second.get());
}

ChestBlockEntity* BlockEntityStore::getChest(
    const BlockPosition& position) noexcept
{
    const auto found = entries_.find(position);
    return found == entries_.end()
        ? nullptr
        : dynamic_cast<ChestBlockEntity*>(found->second.get());
}

const ChestBlockEntity* BlockEntityStore::getChest(
    const BlockPosition& position) const noexcept
{
    const auto found = entries_.find(position);
    return found == entries_.end()
        ? nullptr
        : dynamic_cast<const ChestBlockEntity*>(found->second.get());
}

FurnaceBlockEntity& BlockEntityStore::getOrCreateFurnace(
    const BlockPosition& position)
{
    if (FurnaceBlockEntity* existing = getFurnace(position))
        return *existing;
    auto value = types_.create(FurnaceType);
    auto* furnace = dynamic_cast<FurnaceBlockEntity*>(value.get());
    if (furnace == nullptr)
        throw std::logic_error("Furnace block entity factory returned wrong type");
    entries_.insert_or_assign(position, std::move(value));
    return *furnace;
}

ChestBlockEntity& BlockEntityStore::getOrCreateChest(
    const BlockPosition& position)
{
    if (ChestBlockEntity* existing = getChest(position))
        return *existing;
    auto value = types_.create(ChestType);
    auto* chest = dynamic_cast<ChestBlockEntity*>(value.get());
    if (chest == nullptr)
        throw std::logic_error("Chest block entity factory returned wrong type");
    entries_.insert_or_assign(position, std::move(value));
    return *chest;
}

std::vector<ItemStack> BlockEntityStore::copyContainerContents(
    const BlockPosition& position) const
{
    const auto found = entries_.find(position);
    if (found == entries_.end())
        return {};
    const std::span<const ItemStack> items = found->second->containerItems();
    return {items.begin(), items.end()};
}

bool BlockEntityStore::erase(const BlockPosition& position) noexcept
{
    return entries_.erase(position) != 0;
}

void BlockEntityStore::clear() noexcept
{
    entries_.clear();
}

const std::unordered_map<
    BlockPosition,
    std::unique_ptr<BlockEntity>,
    BlockPositionHash
>& BlockEntityStore::entries() const noexcept
{
    return entries_;
}

std::unordered_map<
    BlockPosition,
    std::unique_ptr<BlockEntity>,
    BlockPositionHash
>& BlockEntityStore::entries() noexcept
{
    return entries_;
}

const BlockEntityTypeRegistry& BlockEntityStore::types() const noexcept
{
    return types_;
}

std::vector<BlockEntityRecord> BlockEntityStore::snapshot() const
{
    std::vector<BlockEntityRecord> result;
    result.reserve(entries_.size());
    for (const auto& [position, value] : entries_)
    {
        result.push_back({
            position,
            value->typeId(),
            value->savePersistentData()
        });
    }
    return result;
}

void BlockEntityStore::restore(std::vector<BlockEntityRecord> records)
{
    entries_.clear();
    entries_.reserve(records.size());
    for (BlockEntityRecord& record : records)
    {
        std::unique_ptr<BlockEntity> value = types_.create(record.type);
        if (!value)
        {
            throw std::runtime_error(
                "Unknown block entity type: " + record.type.toString()
            );
        }
        value->loadPersistentData(record.data);
        entries_.insert_or_assign(record.position, std::move(value));
    }
}
