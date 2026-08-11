#pragma once

#include "Block.h"
#include "ToolProperties.h"

#include <cstdint>

// Block items retain their BlockType numeric value. Standalone items begin at
// 256, matching Beta's split between block IDs and Item.shiftedIndex values.
enum class ItemType : std::uint16_t
{
    Empty = 0,

    Stick = 256,
    Coal,
    Diamond,
    IronIngot,
    GoldIngot,
    Flint,
    RedstoneDust,
    ClayBall,
    LapisLazuli,
    Seeds,
    OakSapling,
    SpruceSapling,
    BirchSapling,
    Charcoal,
    Brick,

    WoodenShovel,
    WoodenPickaxe,
    WoodenAxe,
    StoneShovel,
    StonePickaxe,
    StoneAxe,
    IronShovel,
    IronPickaxe,
    IronAxe,
    DiamondShovel,
    DiamondPickaxe,
    DiamondAxe,
    GoldenShovel,
    GoldenPickaxe,
    GoldenAxe,

    Apple,
    Bread,
    Carrot,
    Potato,
    BakedPotato,
    CookedBeef,
    Shield,
    IronHelmet,
    IronChestplate,
    IronLeggings,
    IronBoots,
    DiamondHelmet,
    DiamondChestplate,
    DiamondLeggings,
    DiamondBoots,

    String,
    GlowstoneDust,
    Snowball,
    WheatItem,
    BeetrootItem,
    BeetrootSeeds,
    MelonSlice,
    CocoaBeans,
    NetherBrickItem,
    Book,
    JungleSapling,
    AcaciaSapling,
    DarkOakSapling,

    Arrow,
    RawBeef,
    BlazeRod,
    Bone,
    RawChicken,
    Dye,
    Emerald,
    EnderPearl,
    Feather,
    RawFish,
    GhastTear,
    GlassBottle,
    GoldNugget,
    Gunpowder,
    Leather,
    MagmaCream,
    RawMutton,
    NetherStar,
    RawPorkchop,
    PrismarineCrystals,
    PrismarineShard,
    RawRabbit,
    RabbitFoot,
    RabbitHide,
    RottenFlesh,
    ShulkerShell,
    SlimeBall,
    SpiderEye,
    Sugar,
    TippedArrow,
    TotemOfUndying
};

enum class ArmorSlot : std::uint8_t
{
    None,
    Head,
    Chest,
    Legs,
    Feet
};

struct ItemProperties
{
    const char* name = "Empty";
    int atlasColumn = -1;
    int atlasRow = -1;
    std::uint8_t maximumStackSize = 64;
    std::uint16_t maximumDamage = 0;
    ToolProperties tool{};
    int foodPoints = 0;
    float saturationModifier = 0.0f;
    bool shield = false;
    ArmorSlot armorSlot = ArmorSlot::None;
    int armorPoints = 0;
    float armorToughness = 0.0f;
};

[[nodiscard]] constexpr ItemType itemFromBlock(BlockType block) noexcept
{
    return static_cast<ItemType>(static_cast<std::uint16_t>(block));
}

[[nodiscard]] constexpr bool isBlockItem(ItemType item) noexcept
{
    const std::uint16_t value = static_cast<std::uint16_t>(item);
    return value > 0 &&
           value <= static_cast<std::uint16_t>(BlockType::Cobweb);
}

[[nodiscard]] constexpr BlockType blockFromItem(ItemType item) noexcept
{
    return isBlockItem(item)
        ? static_cast<BlockType>(static_cast<std::uint8_t>(item))
        : BlockType::Air;
}

[[nodiscard]] const ItemProperties& getItemProperties(
    ItemType item
) noexcept;
[[nodiscard]] bool isToolItem(ItemType item) noexcept;
[[nodiscard]] ToolProperties getItemToolProperties(ItemType item) noexcept;

struct ItemStack
{
    ItemType item = ItemType::Empty;
    std::uint8_t count = 0;
    std::uint16_t damage = 0;

    constexpr ItemStack() noexcept = default;

    constexpr ItemStack(
        ItemType itemType,
        std::uint8_t stackCount,
        std::uint16_t itemDamage = 0) noexcept
        : item(itemType), count(stackCount), damage(itemDamage)
    {
    }

    constexpr ItemStack(
        BlockType block,
        std::uint8_t stackCount = 1) noexcept
        : item(itemFromBlock(block)), count(stackCount)
    {
    }

    [[nodiscard]] bool empty() const noexcept;
    void clear() noexcept;
    [[nodiscard]] int maximumStackSize() const noexcept;
    [[nodiscard]] bool canStackWith(const ItemStack& other) const noexcept;
    void damageItem(int amount) noexcept;
};
