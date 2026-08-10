#include "content/ContentCatalog.h"

#include <algorithm>
#include <charconv>
#include <stdexcept>
#include <utility>

namespace mc::content
{
namespace
{
const ContentCatalog* ActiveCatalog = nullptr;

bool isFrontFace(BlockFace face, std::uint8_t metadata) noexcept
{
    return (metadata == 2 && face == BlockFace::Back) ||
           (metadata == 3 && face == BlockFace::Front) ||
           (metadata == 4 && face == BlockFace::Left) ||
           (metadata == 5 && face == BlockFace::Right);
}
}

bool BlockPropertyDefinition::accepts(
    std::uint8_t properties) const noexcept
{
    const std::uint8_t value = properties & mask;
    return mask != 0 && value >= minimumValue && value <= maximumValue;
}

bool BlockStateSchema::accepts(BlockState state) const noexcept
{
    std::uint8_t knownMask = 0;
    for (const BlockPropertyDefinition& property : properties)
    {
        if ((knownMask & property.mask) != 0 || !property.accepts(state.properties()))
            return false;
        knownMask |= property.mask;
    }
    return (state.properties() & static_cast<std::uint8_t>(~knownMask)) == 0;
}

const core::ResourceLocation* BlockTextures::resolve(
    BlockFace face,
    std::uint8_t metadata) const noexcept
{
    const auto value = [](const auto& candidate) -> const core::ResourceLocation*
    {
        return candidate ? &*candidate : nullptr;
    };

    if (face == BlockFace::Top)
    {
        if (const auto* texture = value(top)) return texture;
    }
    else if (face == BlockFace::Bottom)
    {
        if (const auto* texture = value(bottom)) return texture;
    }
    else if (horizontalFacing)
    {
        if (isFrontFace(face, metadata))
        {
            if (const auto* texture = value(front)) return texture;
        }
        if (const auto* texture = value(side)) return texture;
    }
    else
    {
        const std::optional<core::ResourceLocation>* candidate = nullptr;
        switch (face)
        {
            case BlockFace::Front: candidate = &front; break;
            case BlockFace::Back: candidate = &back; break;
            case BlockFace::Left: candidate = &left; break;
            case BlockFace::Right: candidate = &right; break;
            case BlockFace::Bottom:
            case BlockFace::Top: break;
        }
        if (candidate != nullptr)
        {
            if (const auto* texture = value(*candidate)) return texture;
        }
        if (const auto* texture = value(side)) return texture;
    }

    if (const auto* texture = value(all)) return texture;
    if (const auto* texture = value(side)) return texture;
    return nullptr;
}

ContentCatalog::ContentCatalog()
    : blocks_(core::ResourceLocation("minecraft:blocks")),
      items_(core::ResourceLocation("minecraft:items")),
      entityTypes_(core::ResourceLocation("minecraft:entity_types")),
      blockEntityTypes_(core::ResourceLocation("minecraft:block_entity_types")),
      lootTables_(core::ResourceLocation("minecraft:loot_tables"))
{
}

BlockDefinition& ContentCatalog::registerBlock(
    core::ResourceLocation name,
    BlockDefinition definition)
{
    const BlockState defaultState(
        definition.legacyType.value_or(BlockType::Air),
        definition.stateSchema.defaultProperties
    );
    if (!definition.stateSchema.accepts(defaultState))
        throw std::invalid_argument("Block has an invalid default state");
    for (const BlockPropertyDefinition& property :
         definition.stateSchema.properties)
    {
        if (property.minimumValue > property.maximumValue ||
            (property.minimumValue & property.mask) != property.minimumValue ||
            (property.maximumValue & property.mask) != property.maximumValue ||
            (!property.valueNames.empty() &&
             property.valueNames.size() !=
                static_cast<std::size_t>(property.maximumValue -
                                         property.minimumValue + 1U)))
        {
            throw std::invalid_argument("Block has an invalid state property");
        }
    }

    if (definition.legacyType)
    {
        const std::uint8_t legacy =
            static_cast<std::uint8_t>(*definition.legacyType);
        if (legacyBlocks_.contains(legacy))
            throw std::invalid_argument("Legacy block is already registered");
        legacyBlocks_.emplace(legacy, name);
    }
    return blocks_.registerValue(std::move(name), std::move(definition));
}

ItemDefinition& ContentCatalog::registerItem(
    core::ResourceLocation name,
    ItemDefinition definition)
{
    if (definition.legacyType)
    {
        const std::uint16_t legacy =
            static_cast<std::uint16_t>(*definition.legacyType);
        if (legacyItems_.contains(legacy))
            throw std::invalid_argument("Legacy item is already registered");
        legacyItems_.emplace(legacy, name);
    }
    return items_.registerValue(std::move(name), std::move(definition));
}

EntityTypeDefinition& ContentCatalog::registerEntityType(
    core::ResourceLocation name,
    EntityTypeDefinition definition)
{
    return entityTypes_.registerValue(std::move(name), std::move(definition));
}

BlockEntityTypeDefinition& ContentCatalog::registerBlockEntityType(
    core::ResourceLocation name,
    BlockEntityTypeDefinition definition)
{
    return blockEntityTypes_.registerValue(
        std::move(name), std::move(definition)
    );
}

LootTableDefinition& ContentCatalog::registerLootTable(
    core::ResourceLocation name,
    LootTableDefinition definition)
{
    return lootTables_.registerValue(std::move(name), std::move(definition));
}

void ContentCatalog::freeze()
{
    blocks_.freeze();
    items_.freeze();
    entityTypes_.freeze();
    blockEntityTypes_.freeze();
    lootTables_.freeze();

    // Compatibility constructors encode legacy BlockType values directly as
    // runtime IDs. Built-in content is always the first module and must retain
    // that mapping until all legacy gameplay call sites are removed.
    for (const auto& [legacy, name] : legacyBlocks_)
    {
        if (blocks_.runtimeId(name) != legacy)
            throw std::logic_error("Legacy block/runtime ID mapping changed");
        legacyBlockDefinitions_[legacy] = blocks_.find(name);
    }
    for (const auto& block : blocks_.entries())
    {
        if (lootTables_.find(block.value.behaviour.lootTable) == nullptr)
        {
            throw std::logic_error(
                "Block references unknown loot table: " +
                block.value.behaviour.lootTable.toString()
            );
        }
    }
}

const BlockDefinition* ContentCatalog::block(BlockType legacyType) const noexcept
{
    const std::uint8_t legacy = static_cast<std::uint8_t>(legacyType);
    if (const BlockDefinition* cached = legacyBlockDefinitions_[legacy])
        return cached;
    const auto found = legacyBlocks_.find(legacy);
    return found == legacyBlocks_.end() ? nullptr : blocks_.find(found->second);
}

const BlockDefinition* ContentCatalog::block(BlockState state) const noexcept
{
    const auto* entry = blocks_.entry(state.blockRuntimeId());
    return entry == nullptr ? nullptr : &entry->value;
}

bool ContentCatalog::isValidState(BlockState state) const noexcept
{
    const BlockDefinition* definition = block(state);
    return definition != nullptr && definition->stateSchema.accepts(state);
}

BlockState ContentCatalog::defaultState(BlockType legacyType) const noexcept
{
    const auto found = legacyBlocks_.find(static_cast<std::uint8_t>(legacyType));
    return found == legacyBlocks_.end()
        ? BlockState{}
        : defaultState(found->second);
}

BlockState ContentCatalog::defaultState(
    const core::ResourceLocation& name) const noexcept
{
    const auto* entry = blocks_.entry(name);
    if (entry == nullptr || entry->runtimeId == core::InvalidRuntimeId)
        return {};
    return BlockState::fromRuntimeId(
        entry->runtimeId,
        entry->value.stateSchema.defaultProperties
    );
}

std::optional<BlockState> ContentCatalog::state(
    const core::ResourceLocation& name,
    std::uint8_t properties) const noexcept
{
    const auto* entry = blocks_.entry(name);
    if (entry == nullptr || entry->runtimeId == core::InvalidRuntimeId)
        return std::nullopt;
    const BlockState result = BlockState::fromRuntimeId(
        entry->runtimeId,
        properties
    );
    return entry->value.stateSchema.accepts(result)
        ? std::optional<BlockState>(result)
        : std::nullopt;
}

std::optional<BlockState> ContentCatalog::state(
    const core::ResourceLocation& name,
    std::span<const std::pair<std::string, std::string>> properties) const
{
    const auto* entry = blocks_.entry(name);
    if (entry == nullptr || entry->runtimeId == core::InvalidRuntimeId)
        return std::nullopt;

    std::uint8_t encoded = entry->value.stateSchema.defaultProperties;
    for (const auto& [propertyName, textValue] : properties)
    {
        const auto found = std::find_if(
            entry->value.stateSchema.properties.begin(),
            entry->value.stateSchema.properties.end(),
            [&propertyName](const BlockPropertyDefinition& property)
            {
                return property.name == propertyName;
            }
        );
        if (found == entry->value.stateSchema.properties.end())
            return std::nullopt;

        std::uint8_t value = 0;
        if (!found->valueNames.empty())
        {
            const auto valueName = std::find(
                found->valueNames.begin(), found->valueNames.end(), textValue
            );
            if (valueName == found->valueNames.end())
                return std::nullopt;
            value = static_cast<std::uint8_t>(
                found->minimumValue +
                std::distance(found->valueNames.begin(), valueName)
            );
        }
        else
        {
            unsigned int parsed = 0;
            const auto result = std::from_chars(
                textValue.data(), textValue.data() + textValue.size(), parsed
            );
            if (result.ec != std::errc{} ||
                result.ptr != textValue.data() + textValue.size() ||
                parsed > 255U)
            {
                return std::nullopt;
            }
            value = static_cast<std::uint8_t>(parsed);
        }
        if (value < found->minimumValue || value > found->maximumValue)
            return std::nullopt;
        encoded = static_cast<std::uint8_t>(
            (encoded & static_cast<std::uint8_t>(~found->mask)) |
            (value & found->mask)
        );
    }
    return state(name, encoded);
}

std::vector<std::pair<std::string, std::string>>
ContentCatalog::serializeStateProperties(BlockState state) const
{
    std::vector<std::pair<std::string, std::string>> result;
    const BlockDefinition* definition = block(state);
    if (definition == nullptr)
        return result;
    result.reserve(definition->stateSchema.properties.size());
    for (const BlockPropertyDefinition& property :
         definition->stateSchema.properties)
    {
        const std::uint8_t value = state.properties() & property.mask;
        std::string text;
        if (!property.valueNames.empty() &&
            value >= property.minimumValue && value <= property.maximumValue)
        {
            text = property.valueNames[
                static_cast<std::size_t>(value - property.minimumValue)
            ];
        }
        else
        {
            text = std::to_string(value);
        }
        result.emplace_back(property.name, std::move(text));
    }
    return result;
}

const core::ResourceLocation* ContentCatalog::blockName(
    BlockState state) const noexcept
{
    const auto* entry = blocks_.entry(state.blockRuntimeId());
    return entry == nullptr ? nullptr : &entry->name;
}

std::optional<BlockType> ContentCatalog::legacyBlock(
    BlockState state) const noexcept
{
    const BlockDefinition* definition = block(state);
    return definition == nullptr ? std::nullopt : definition->legacyType;
}

const ItemDefinition* ContentCatalog::item(ItemType legacyType) const noexcept
{
    const auto found = legacyItems_.find(static_cast<std::uint16_t>(legacyType));
    return found == legacyItems_.end() ? nullptr : items_.find(found->second);
}

const ItemDefinition* ContentCatalog::item(
    const core::ResourceLocation& name) const noexcept
{
    return items_.find(name);
}

std::optional<ItemType> ContentCatalog::legacyItem(
    const core::ResourceLocation& name) const noexcept
{
    const ItemDefinition* definition = item(name);
    return definition == nullptr ? std::nullopt : definition->legacyType;
}

const core::ResourceLocation* ContentCatalog::itemName(
    ItemType legacyType) const noexcept
{
    const auto found = legacyItems_.find(static_cast<std::uint16_t>(legacyType));
    if (found == legacyItems_.end())
        return nullptr;
    const auto* entry = items_.entry(found->second);
    return entry == nullptr ? nullptr : &entry->name;
}

const core::Registry<BlockDefinition>& ContentCatalog::blocks() const noexcept
{
    return blocks_;
}

const core::Registry<ItemDefinition>& ContentCatalog::items() const noexcept
{
    return items_;
}

const core::Registry<EntityTypeDefinition>&
ContentCatalog::entityTypes() const noexcept
{
    return entityTypes_;
}

const core::Registry<BlockEntityTypeDefinition>&
ContentCatalog::blockEntityTypes() const noexcept
{
    return blockEntityTypes_;
}

const LootTableDefinition* ContentCatalog::lootTable(
    const core::ResourceLocation& name) const noexcept
{
    return lootTables_.find(name);
}

const core::Registry<LootTableDefinition>&
ContentCatalog::lootTables() const noexcept
{
    return lootTables_;
}

bool ContentCatalog::frozen() const noexcept
{
    return blocks_.frozen() && items_.frozen() &&
           entityTypes_.frozen() && blockEntityTypes_.frozen() &&
           lootTables_.frozen();
}

void ContentCatalog::activate() const noexcept
{
    ActiveCatalog = this;
}

const ContentCatalog* ContentCatalog::active() noexcept
{
    return ActiveCatalog;
}
}
