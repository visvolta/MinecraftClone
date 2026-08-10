#include "Item.h"

#include "content/ContentCatalog.h"

#include <algorithm>

namespace
{
constexpr ItemProperties Empty{"Empty", -1, -1, 0, 0, {}};
constexpr ItemProperties BlockItem{"Block", -1, -1, 64, 0, {}};

constexpr ItemProperties Stick{
    "Stick", 5, 3, 64, 0, {}
};
constexpr ItemProperties Coal{
    "Coal", 7, 0, 64, 0, {}
};
constexpr ItemProperties Diamond{
    "Diamond", 7, 3, 64, 0, {}
};
constexpr ItemProperties IronIngot{
    "Iron Ingot", 7, 1, 64, 0, {}
};
constexpr ItemProperties GoldIngot{
    "Gold Ingot", 7, 2, 64, 0, {}
};
constexpr ItemProperties Flint{
    "Flint", 6, 0, 64, 0, {}
};
constexpr ItemProperties Redstone{
    "Redstone", 8, 3, 64, 0, {}
};
constexpr ItemProperties ClayBall{
    "Clay", 9, 3, 64, 0, {}
};
// ItemDye metadata 4: base (14,4) + four rows in Beta's ItemDye mapping.
constexpr ItemProperties Lapis{
    "Lapis Lazuli", 14, 8, 64, 0, {}
};
constexpr ItemProperties Seeds{
    "Seeds", 9, 0, 64, 0, {}
};
constexpr ItemProperties OakSapling{
    "Oak Sapling", 15, 0, 64, 0, {}
};
constexpr ItemProperties SpruceSapling{
    "Spruce Sapling", 15, 1, 64, 0, {}
};
constexpr ItemProperties BirchSapling{
    "Birch Sapling", 15, 2, 64, 0, {}
};
constexpr ItemProperties Charcoal{
    "Charcoal", 7, 0, 64, 0, {}
};
constexpr ItemProperties Brick{
    "Brick", 6, 1, 64, 0, {}
};

constexpr ItemProperties WoodenShovel{
    "Wooden Shovel", 0, 5, 1, 59,
    {ToolType::Shovel, ToolMaterial::Wood, 2.0f, 0}
};
constexpr ItemProperties WoodenPickaxe{
    "Wooden Pickaxe", 0, 6, 1, 59,
    {ToolType::Pickaxe, ToolMaterial::Wood, 2.0f, 0}
};
constexpr ItemProperties WoodenAxe{
    "Wooden Axe", 0, 7, 1, 59,
    {ToolType::Axe, ToolMaterial::Wood, 2.0f, 0}
};
constexpr ItemProperties StoneShovel{
    "Stone Shovel", 1, 5, 1, 131,
    {ToolType::Shovel, ToolMaterial::Stone, 4.0f, 1}
};
constexpr ItemProperties StonePickaxe{
    "Stone Pickaxe", 1, 6, 1, 131,
    {ToolType::Pickaxe, ToolMaterial::Stone, 4.0f, 1}
};
constexpr ItemProperties StoneAxe{
    "Stone Axe", 1, 7, 1, 131,
    {ToolType::Axe, ToolMaterial::Stone, 4.0f, 1}
};
constexpr ItemProperties IronShovel{
    "Iron Shovel", 2, 5, 1, 250,
    {ToolType::Shovel, ToolMaterial::Iron, 6.0f, 2}
};
constexpr ItemProperties IronPickaxe{
    "Iron Pickaxe", 2, 6, 1, 250,
    {ToolType::Pickaxe, ToolMaterial::Iron, 6.0f, 2}
};
constexpr ItemProperties IronAxe{
    "Iron Axe", 2, 7, 1, 250,
    {ToolType::Axe, ToolMaterial::Iron, 6.0f, 2}
};
constexpr ItemProperties DiamondShovel{
    "Diamond Shovel", 3, 5, 1, 1561,
    {ToolType::Shovel, ToolMaterial::Diamond, 8.0f, 3}
};
constexpr ItemProperties DiamondPickaxe{
    "Diamond Pickaxe", 3, 6, 1, 1561,
    {ToolType::Pickaxe, ToolMaterial::Diamond, 8.0f, 3}
};
constexpr ItemProperties DiamondAxe{
    "Diamond Axe", 3, 7, 1, 1561,
    {ToolType::Axe, ToolMaterial::Diamond, 8.0f, 3}
};
constexpr ItemProperties GoldenShovel{
    "Golden Shovel", 4, 5, 1, 32,
    {ToolType::Shovel, ToolMaterial::Gold, 12.0f, 0}
};
constexpr ItemProperties GoldenPickaxe{
    "Golden Pickaxe", 4, 6, 1, 32,
    {ToolType::Pickaxe, ToolMaterial::Gold, 12.0f, 0}
};
constexpr ItemProperties GoldenAxe{
    "Golden Axe", 4, 7, 1, 32,
    {ToolType::Axe, ToolMaterial::Gold, 12.0f, 0}
};
constexpr ItemProperties Apple{
    "Apple", -1, -1, 64, 0, {}, 4, 0.3f, false
};
constexpr ItemProperties Bread{
    "Bread", -1, -1, 64, 0, {}, 5, 0.6f, false
};
constexpr ItemProperties Carrot{
    "Carrot", -1, -1, 64, 0, {}, 3, 0.6f, false
};
constexpr ItemProperties Potato{
    "Potato", -1, -1, 64, 0, {}, 1, 0.3f, false
};
constexpr ItemProperties BakedPotato{
    "Baked Potato", -1, -1, 64, 0, {}, 5, 0.6f, false
};
constexpr ItemProperties CookedBeef{
    "Steak", -1, -1, 64, 0, {}, 8, 0.8f, false
};
constexpr ItemProperties Shield{
    "Shield", -1, -1, 1, 336, {}, 0, 0.0f, true
};
constexpr ItemProperties IronHelmet{
    "Iron Helmet", -1, -1, 1, 165, {}, 0, 0.0f, false,
    ArmorSlot::Head, 2, 0.0f
};
constexpr ItemProperties IronChestplate{
    "Iron Chestplate", -1, -1, 1, 240, {}, 0, 0.0f, false,
    ArmorSlot::Chest, 6, 0.0f
};
constexpr ItemProperties IronLeggings{
    "Iron Leggings", -1, -1, 1, 225, {}, 0, 0.0f, false,
    ArmorSlot::Legs, 5, 0.0f
};
constexpr ItemProperties IronBoots{
    "Iron Boots", -1, -1, 1, 195, {}, 0, 0.0f, false,
    ArmorSlot::Feet, 2, 0.0f
};
constexpr ItemProperties DiamondHelmet{
    "Diamond Helmet", -1, -1, 1, 363, {}, 0, 0.0f, false,
    ArmorSlot::Head, 3, 2.0f
};
constexpr ItemProperties DiamondChestplate{
    "Diamond Chestplate", -1, -1, 1, 528, {}, 0, 0.0f, false,
    ArmorSlot::Chest, 8, 2.0f
};
constexpr ItemProperties DiamondLeggings{
    "Diamond Leggings", -1, -1, 1, 495, {}, 0, 0.0f, false,
    ArmorSlot::Legs, 6, 2.0f
};
constexpr ItemProperties DiamondBoots{
    "Diamond Boots", -1, -1, 1, 429, {}, 0, 0.0f, false,
    ArmorSlot::Feet, 3, 2.0f
};
}

const ItemProperties& getItemProperties(ItemType item) noexcept
{
    if (const mc::content::ContentCatalog* catalog =
            mc::content::ContentCatalog::active())
    {
        if (const mc::content::ItemDefinition* definition =
                catalog->item(item))
        {
            return definition->properties;
        }
    }
    if (isBlockItem(item))
        return BlockItem;

    switch (item)
    {
        case ItemType::Stick: return Stick;
        case ItemType::Coal: return Coal;
        case ItemType::Diamond: return Diamond;
        case ItemType::IronIngot: return IronIngot;
        case ItemType::GoldIngot: return GoldIngot;
        case ItemType::Flint: return Flint;
        case ItemType::RedstoneDust: return Redstone;
        case ItemType::ClayBall: return ClayBall;
        case ItemType::LapisLazuli: return Lapis;
        case ItemType::Seeds: return Seeds;
        case ItemType::OakSapling: return OakSapling;
        case ItemType::SpruceSapling: return SpruceSapling;
        case ItemType::BirchSapling: return BirchSapling;
        case ItemType::Charcoal: return Charcoal;
        case ItemType::Brick: return Brick;
        case ItemType::WoodenShovel: return WoodenShovel;
        case ItemType::WoodenPickaxe: return WoodenPickaxe;
        case ItemType::WoodenAxe: return WoodenAxe;
        case ItemType::StoneShovel: return StoneShovel;
        case ItemType::StonePickaxe: return StonePickaxe;
        case ItemType::StoneAxe: return StoneAxe;
        case ItemType::IronShovel: return IronShovel;
        case ItemType::IronPickaxe: return IronPickaxe;
        case ItemType::IronAxe: return IronAxe;
        case ItemType::DiamondShovel: return DiamondShovel;
        case ItemType::DiamondPickaxe: return DiamondPickaxe;
        case ItemType::DiamondAxe: return DiamondAxe;
        case ItemType::GoldenShovel: return GoldenShovel;
        case ItemType::GoldenPickaxe: return GoldenPickaxe;
        case ItemType::GoldenAxe: return GoldenAxe;
        case ItemType::Apple: return Apple;
        case ItemType::Bread: return Bread;
        case ItemType::Carrot: return Carrot;
        case ItemType::Potato: return Potato;
        case ItemType::BakedPotato: return BakedPotato;
        case ItemType::CookedBeef: return CookedBeef;
        case ItemType::Shield: return Shield;
        case ItemType::IronHelmet: return IronHelmet;
        case ItemType::IronChestplate: return IronChestplate;
        case ItemType::IronLeggings: return IronLeggings;
        case ItemType::IronBoots: return IronBoots;
        case ItemType::DiamondHelmet: return DiamondHelmet;
        case ItemType::DiamondChestplate: return DiamondChestplate;
        case ItemType::DiamondLeggings: return DiamondLeggings;
        case ItemType::DiamondBoots: return DiamondBoots;
        case ItemType::Empty: return Empty;
    }
    return Empty;
}

bool isToolItem(ItemType item) noexcept
{
    return getItemProperties(item).tool.type != ToolType::None;
}

ToolProperties getItemToolProperties(ItemType item) noexcept
{
    return getItemProperties(item).tool;
}

bool ItemStack::empty() const noexcept
{
    return item == ItemType::Empty || count == 0;
}

void ItemStack::clear() noexcept
{
    item = ItemType::Empty;
    count = 0;
    damage = 0;
}

int ItemStack::maximumStackSize() const noexcept
{
    return static_cast<int>(getItemProperties(item).maximumStackSize);
}

bool ItemStack::canStackWith(const ItemStack& other) const noexcept
{
    return !empty() && !other.empty() && item == other.item &&
           damage == other.damage && maximumStackSize() > 1;
}

void ItemStack::damageItem(int amount) noexcept
{
    const int maximumDamage = getItemProperties(item).maximumDamage;
    if (maximumDamage <= 0 || amount <= 0 || empty())
        return;

    damage = static_cast<std::uint16_t>(
        std::min<int>(damage + amount, 65535)
    );

    // Beta ItemStack.damageItem breaks after damage exceeds maxDamage.
    if (damage > maximumDamage)
        clear();
}
