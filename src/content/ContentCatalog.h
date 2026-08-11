#pragma once

#include "Block.h"
#include "BlockProperties.h"
#include "BlockShape.h"
#include "Item.h"
#include "TextureAtlas.h"
#include "core/Registry.h"

#include <cstdint>
#include <array>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mc::content
{
enum class BlockTint
{
    None,
    Grass,
    Foliage,
    SpruceFoliage,
    BirchFoliage
};

enum class RenderLayer
{
    Solid,
    CutoutMipped,
    Cutout,
    Translucent
};

enum class BlockDropRule
{
    None,
    Self,
    Cobblestone,
    Dirt,
    FlintOrGravel,
    ClayBalls,
    Coal,
    Diamond,
    Redstone,
    Lapis,
    Seeds,
    OakSapling,
    SpruceSapling,
    BirchSapling,
    JungleSapling,
    AcaciaSapling,
    DarkOakSapling,
    Furnace,
    Farmland,
    GlassLike,
    Snowball,
    GlowstoneDust,
    RedstoneDust,
    WheatCrop,
    CarrotCrop,
    PotatoCrop,
    BeetrootCrop,
    MelonSlices,
    CocoaBeans,
    Books,
    CobwebString,
    DeadBushSticks,
    MushroomCap
};

struct BlockTraits
{
    bool liquid = false;
    bool leaf = false;
    bool plant = false;
    bool ladder = false;
    bool cutout = false;
    bool translucent = false;
    bool opaque = false;
    bool solid = false;
};

struct BlockBehaviour
{
    BlockProperties breaking;
    const BlockShapeDefinition* shape = nullptr;
    BlockTraits traits;
    BlockTint tint = BlockTint::None;
    BlockDropRule dropRule = BlockDropRule::Self;
    core::ResourceLocation lootTable{"minecraft:self"};
    std::uint8_t lightOpacity = 15;
    std::uint8_t lightEmission = 0;
};

struct BlockPropertyDefinition
{
    std::string name;
    std::uint16_t mask = 0;
    std::uint16_t minimumValue = 0;
    std::uint16_t maximumValue = 0;
    std::vector<std::string> valueNames;

    [[nodiscard]] bool accepts(std::uint16_t properties) const noexcept;
};

struct BlockStateSchema
{
    std::uint16_t defaultProperties = 0;
    std::vector<BlockPropertyDefinition> properties;
    // Resource-pack blocks use an explicit compact state table. This supports
    // arbitrary 1.12 properties without trying to squeeze every combination
    // into the four legacy metadata bits.
    std::vector<std::vector<std::pair<std::string, std::string>>> states;

    [[nodiscard]] bool accepts(BlockState state) const noexcept;
    [[nodiscard]] std::size_t stateCount() const noexcept;
};

struct BlockTextures
{
    std::optional<core::ResourceLocation> all;
    std::optional<core::ResourceLocation> side;
    std::optional<core::ResourceLocation> top;
    std::optional<core::ResourceLocation> bottom;
    std::optional<core::ResourceLocation> front;
    std::optional<core::ResourceLocation> back;
    std::optional<core::ResourceLocation> left;
    std::optional<core::ResourceLocation> right;
    std::optional<core::ResourceLocation> sideOverlay;
    bool horizontalFacing = false;

    [[nodiscard]] const core::ResourceLocation* resolve(
        BlockFace face,
        std::uint16_t metadata
    ) const noexcept;
};

struct BlockDefinition
{
    std::optional<BlockType> legacyType;
    std::string displayName;
    BlockTextures textures;
    BlockStateSchema stateSchema;
    BlockBehaviour behaviour;
    RenderLayer renderLayer = RenderLayer::Solid;
};

struct ItemDefinition
{
    std::optional<ItemType> legacyType;
    std::string displayName;
    std::optional<core::ResourceLocation> placedBlock;
    ItemProperties properties;
};

struct EntityTypeDefinition
{
    std::string displayName;
};

struct BlockEntityTypeDefinition
{
    std::string displayName;
    std::uint32_t dataVersion = 1;
};

struct LootTableDefinition
{
    BlockDropRule rule = BlockDropRule::Self;
};

class ContentCatalog
{
public:
    ContentCatalog();

    BlockDefinition& registerBlock(
        core::ResourceLocation name,
        BlockDefinition definition
    );
    ItemDefinition& registerItem(
        core::ResourceLocation name,
        ItemDefinition definition
    );
    EntityTypeDefinition& registerEntityType(
        core::ResourceLocation name,
        EntityTypeDefinition definition
    );
    BlockEntityTypeDefinition& registerBlockEntityType(
        core::ResourceLocation name,
        BlockEntityTypeDefinition definition
    );
    LootTableDefinition& registerLootTable(
        core::ResourceLocation name,
        LootTableDefinition definition
    );
    void freeze();

    [[nodiscard]] const BlockDefinition* block(BlockType legacyType) const noexcept;
    [[nodiscard]] const BlockDefinition* block(BlockState state) const noexcept;
    [[nodiscard]] bool isValidState(BlockState state) const noexcept;
    [[nodiscard]] BlockState defaultState(BlockType legacyType) const noexcept;
    [[nodiscard]] BlockState defaultState(
        const core::ResourceLocation& name
    ) const noexcept;
    [[nodiscard]] std::optional<BlockState> state(
        const core::ResourceLocation& name,
        std::uint16_t properties = 0
    ) const noexcept;
    [[nodiscard]] std::optional<BlockState> state(
        const core::ResourceLocation& name,
        std::span<const std::pair<std::string, std::string>> properties
    ) const;
    [[nodiscard]] std::vector<std::pair<std::string, std::string>>
        serializeStateProperties(BlockState state) const;
    [[nodiscard]] const core::ResourceLocation* blockName(
        BlockState state
    ) const noexcept;
    [[nodiscard]] std::optional<BlockType> legacyBlock(
        BlockState state
    ) const noexcept;
    [[nodiscard]] const ItemDefinition* item(ItemType legacyType) const noexcept;
    [[nodiscard]] const ItemDefinition* item(
        const core::ResourceLocation& name
    ) const noexcept;
    [[nodiscard]] std::optional<ItemType> legacyItem(
        const core::ResourceLocation& name
    ) const noexcept;
    [[nodiscard]] const core::ResourceLocation* itemName(
        ItemType legacyType
    ) const noexcept;
    [[nodiscard]] const core::Registry<BlockDefinition>& blocks() const noexcept;
    [[nodiscard]] const core::Registry<ItemDefinition>& items() const noexcept;
    [[nodiscard]] const core::Registry<EntityTypeDefinition>&
        entityTypes() const noexcept;
    [[nodiscard]] const core::Registry<BlockEntityTypeDefinition>&
        blockEntityTypes() const noexcept;
    [[nodiscard]] const LootTableDefinition* lootTable(
        const core::ResourceLocation& name
    ) const noexcept;
    [[nodiscard]] const core::Registry<LootTableDefinition>&
        lootTables() const noexcept;
    [[nodiscard]] bool frozen() const noexcept;
    void activate() const noexcept;
    [[nodiscard]] static const ContentCatalog* active() noexcept;

private:
    core::Registry<BlockDefinition> blocks_;
    core::Registry<ItemDefinition> items_;
    core::Registry<EntityTypeDefinition> entityTypes_;
    core::Registry<BlockEntityTypeDefinition> blockEntityTypes_;
    core::Registry<LootTableDefinition> lootTables_;
    std::unordered_map<std::uint8_t, core::ResourceLocation> legacyBlocks_;
    std::unordered_map<std::uint16_t, core::ResourceLocation> legacyItems_;
    // Frozen registries never move again. Legacy gameplay and hot rendering
    // paths can therefore use a direct ID lookup instead of two hash-table
    // probes for every block face, light sample, and collision query.
    std::array<const BlockDefinition*, 256> legacyBlockDefinitions_{};
};
}
