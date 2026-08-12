#include "BlockEntity.h"
#include "content/ContentCatalog.h"

#include <algorithm>
#include <functional>
#include <stdexcept>
#include <utility>

namespace
{
const mc::core::ResourceLocation FurnaceType("minecraft:furnace");
const mc::core::ResourceLocation ChestType("minecraft:chest");
const mc::core::ResourceLocation SpawnerType("minecraft:mob_spawner");

std::optional<mc::core::ResourceLocation> blockEntityTypeForState(
    mc::content::BlockState state)
{
    const mc::content::ContentCatalog* catalog =
        mc::content::ContentCatalog::active();
    if (catalog == nullptr)
        return std::nullopt;

    const mc::content::BlockDefinition* definition =
        catalog->block(state);
    if (definition == nullptr ||
        !definition->behaviour.blockEntityType)
        return std::nullopt;

    return definition->behaviour.blockEntityType;
}
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

void SpawnerBlockEntity::setMobId(int mobId) noexcept
{
    mobId_ = std::clamp(mobId, 0, 2);
}

int SpawnerBlockEntity::mobId() const noexcept
{
    return mobId_;
}

const mc::core::ResourceLocation& SpawnerBlockEntity::typeId() const noexcept
{
    return SpawnerType;
}

std::unique_ptr<BlockEntity> SpawnerBlockEntity::clone() const
{
    return std::make_unique<SpawnerBlockEntity>(*this);
}

BlockEntityPersistentData SpawnerBlockEntity::savePersistentData() const
{
    BlockEntityPersistentData data;
    data.integers.emplace_back("mob_id", mobId_);
    return data;
}

void SpawnerBlockEntity::loadPersistentData(
    const BlockEntityPersistentData& data)
{
    const auto mob = std::find_if(
        data.integers.begin(), data.integers.end(),
        [](const auto& entry)
        {
            return entry.first == "mob_id";
        });
    setMobId(mob == data.integers.end() ? 0 : mob->second);
}

GenericBlockEntity::GenericBlockEntity(
    mc::core::ResourceLocation type)
    : type_(std::move(type))
{
}

const mc::core::ResourceLocation&
GenericBlockEntity::typeId() const noexcept
{
    return type_;
}

std::unique_ptr<BlockEntity> GenericBlockEntity::clone() const
{
    return std::make_unique<GenericBlockEntity>(*this);
}

BlockEntityPersistentData GenericBlockEntity::savePersistentData() const
{
    return data_;
}

void GenericBlockEntity::loadPersistentData(
    const BlockEntityPersistentData& data)
{
    data_ = data;
}

InventoryBlockEntity::InventoryBlockEntity(
    mc::core::ResourceLocation type,
    std::size_t slotCount)
    : type_(std::move(type)),
      slots_(slotCount)
{
}

const mc::core::ResourceLocation&
InventoryBlockEntity::typeId() const noexcept
{
    return type_;
}

std::unique_ptr<BlockEntity> InventoryBlockEntity::clone() const
{
    return std::make_unique<InventoryBlockEntity>(*this);
}

BlockEntityPersistentData InventoryBlockEntity::savePersistentData() const
{
    BlockEntityPersistentData data = extra_;
    data.items = slots_;
    return data;
}

void InventoryBlockEntity::loadPersistentData(
    const BlockEntityPersistentData& data)
{
    extra_ = data;
    std::fill(slots_.begin(), slots_.end(), ItemStack{});
    std::copy_n(
        data.items.begin(),
        std::min(data.items.size(), slots_.size()),
        slots_.begin()
    );
    extra_.items.clear();
}

std::span<ItemStack> InventoryBlockEntity::containerItems() noexcept
{
    return slots_;
}

std::span<const ItemStack>
InventoryBlockEntity::containerItems() const noexcept
{
    return slots_;
}

std::size_t InventoryBlockEntity::slotCount() const noexcept
{
    return slots_.size();
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
    types.registerType(SpawnerType, []
    {
        return std::make_unique<SpawnerBlockEntity>();
    });

    const auto addSimple =
        [&types](std::string_view name)
    {
        const mc::core::ResourceLocation type(
            "minecraft", std::string(name));
        types.registerType(type, [type]
        {
            return std::make_unique<GenericBlockEntity>(type);
        });
    };

    const auto addInventory =
        [&types](std::string_view name, std::size_t slots)
    {
        const mc::core::ResourceLocation type(
            "minecraft", std::string(name));
        types.registerType(type, [type, slots]
        {
            return std::make_unique<InventoryBlockEntity>(
                type, slots);
        });
    };

    addSimple("ender_chest");
    addInventory("jukebox", 1);
    addInventory("dispenser", 9);
    addInventory("dropper", 9);
    addSimple("sign");
    addSimple("noteblock");
    addSimple("piston");
    addInventory("brewing_stand", 5);
    addSimple("enchanting_table");
    addSimple("end_portal");
    addInventory("beacon", 1);
    addSimple("skull");
    addSimple("daylight_detector");
    addInventory("hopper", 5);
    addSimple("comparator");
    addSimple("flower_pot");
    addSimple("banner");
    addSimple("structure_block");
    addSimple("end_gateway");
    addSimple("command_block");
    addInventory("shulker_box", 27);
    addSimple("bed");

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
    onBlockChanged(
        position,
        mc::content::BlockState(oldBlock),
        mc::content::BlockState(newBlock)
    );
}

void BlockEntityStore::onBlockChanged(
    const BlockPosition& position,
    mc::content::BlockState oldState,
    mc::content::BlockState newState)
{
    const auto oldType = blockEntityTypeForState(oldState);
    const auto newType = blockEntityTypeForState(newState);

    if (oldType == newType)
        return;

    entries_.erase(position);
    if (!newType)
        return;

    std::unique_ptr<BlockEntity> value = types_.create(*newType);
    if (value)
        entries_.emplace(position, std::move(value));
}

BlockEntity* BlockEntityStore::get(
    const BlockPosition& position) noexcept
{
    const auto found = entries_.find(position);
    return found == entries_.end() ? nullptr : found->second.get();
}

const BlockEntity* BlockEntityStore::get(
    const BlockPosition& position) const noexcept
{
    const auto found = entries_.find(position);
    return found == entries_.end() ? nullptr : found->second.get();
}

InventoryBlockEntity* BlockEntityStore::getInventory(
    const BlockPosition& position) noexcept
{
    return dynamic_cast<InventoryBlockEntity*>(get(position));
}

const InventoryBlockEntity* BlockEntityStore::getInventory(
    const BlockPosition& position) const noexcept
{
    return dynamic_cast<const InventoryBlockEntity*>(get(position));
}

BlockEntity* BlockEntityStore::getOrCreateForState(
    const BlockPosition& position,
    mc::content::BlockState state)
{
    if (BlockEntity* current = get(position))
        return current;

    const auto type = blockEntityTypeForState(state);
    if (!type)
        return nullptr;

    std::unique_ptr<BlockEntity> value = types_.create(*type);
    if (!value)
        return nullptr;

    BlockEntity* result = value.get();
    entries_.insert_or_assign(position, std::move(value));
    return result;
}

bool BlockEntityStore::requiresBlockEntity(
    mc::content::BlockState state) noexcept
{
    return blockEntityTypeForState(state).has_value();
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

SpawnerBlockEntity* BlockEntityStore::getSpawner(
    const BlockPosition& position) noexcept
{
    const auto found = entries_.find(position);
    return found == entries_.end()
        ? nullptr
        : dynamic_cast<SpawnerBlockEntity*>(found->second.get());
}

const SpawnerBlockEntity* BlockEntityStore::getSpawner(
    const BlockPosition& position) const noexcept
{
    const auto found = entries_.find(position);
    return found == entries_.end()
        ? nullptr
        : dynamic_cast<const SpawnerBlockEntity*>(found->second.get());
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

SpawnerBlockEntity& BlockEntityStore::getOrCreateSpawner(
    const BlockPosition& position)
{
    if (SpawnerBlockEntity* existing = getSpawner(position))
        return *existing;
    auto value = types_.create(SpawnerType);
    auto* spawner = dynamic_cast<SpawnerBlockEntity*>(value.get());
    if (spawner == nullptr)
        throw std::logic_error("Spawner block entity factory returned wrong type");
    entries_.insert_or_assign(position, std::move(value));
    return *spawner;
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
