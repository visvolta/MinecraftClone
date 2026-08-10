#pragma once
#include <cstdint>

enum class BlockType : std::uint8_t
{
    Air = 0,
    Dirt, Grass, Stone, Cobblestone, Gravel, Water, Bedrock,
    OakLog, OakLeaves, Sand, Clay, IronOre, GoldOre, RedstoneOre,
    DiamondOre, CoalOre, SpruceLeaves, BirchLeaves, SpruceLog, BirchLog,
    BrownMushroom, RedMushroom, TallGrass, Rose, Dandelion,
    MossyCobblestone, Spawner, Chest, Pumpkin, CraftingTable,
    OakPlanks, SprucePlanks, BirchPlanks, Sandstone, Bricks,
    HayBale, Ladder, LapisBlock, LapisOre, IronBlock, GoldBlock,
    WhiteWool, OrangeWool, MagentaWool, LightBlueWool,
    YellowWool, LimeWool, PinkWool, GrayWool, LightGrayWool,
    CyanWool, PurpleWool, BlueWool, BrownWool, GreenWool,
    RedWool, BlackWool, Obsidian, Furnace, LitFurnace,
    Lava,
    JungleLog, JungleLeaves, AcaciaLog, AcaciaLeaves,
    DarkOakLog, DarkOakLeaves,
    JunglePlanks, AcaciaPlanks, DarkOakPlanks,
    Podzol, Mycelium, Snow, Ice, Cactus, SugarCane,
    Farmland, Wheat, Carrots, Potatoes, Beetroots, Glass,
    Netherrack, SoulSand, NetherBricks, Glowstone, EndStone,
    RedstoneWire, RedstoneTorch, Lever, Repeater,
    Piston, StickyPiston, TNT
};

[[nodiscard]] constexpr bool isAir(BlockType b) noexcept { return b == BlockType::Air; }
[[nodiscard]] constexpr bool isLiquid(BlockType b) noexcept { return b == BlockType::Water || b == BlockType::Lava; }
[[nodiscard]] constexpr bool isFurnace(BlockType b) noexcept
{
    return b == BlockType::Furnace || b == BlockType::LitFurnace;
}
[[nodiscard]] constexpr bool isLeaf(BlockType b) noexcept
{
    return b == BlockType::OakLeaves || b == BlockType::SpruceLeaves ||
           b == BlockType::BirchLeaves || b == BlockType::JungleLeaves ||
           b == BlockType::AcaciaLeaves || b == BlockType::DarkOakLeaves;
}
[[nodiscard]] constexpr bool isLog(BlockType b) noexcept
{
    return b == BlockType::OakLog || b == BlockType::SpruceLog ||
           b == BlockType::BirchLog || b == BlockType::JungleLog ||
           b == BlockType::AcaciaLog || b == BlockType::DarkOakLog;
}
[[nodiscard]] constexpr bool isCrop(BlockType b) noexcept
{
    return b == BlockType::Wheat || b == BlockType::Carrots ||
           b == BlockType::Potatoes || b == BlockType::Beetroots;
}
[[nodiscard]] constexpr bool isPlant(BlockType b) noexcept
{
    return b == BlockType::BrownMushroom || b == BlockType::RedMushroom ||
           b == BlockType::TallGrass || b == BlockType::Rose ||
           b == BlockType::Dandelion || b == BlockType::SugarCane ||
           isCrop(b);
}
[[nodiscard]] constexpr bool isCrossModel(BlockType b) noexcept { return isPlant(b); }
[[nodiscard]] constexpr bool isLadder(BlockType b) noexcept { return b == BlockType::Ladder; }
[[nodiscard]] constexpr bool isWool(BlockType b) noexcept
{
    return b >= BlockType::WhiteWool && b <= BlockType::BlackWool;
}
[[nodiscard]] constexpr bool isCutout(BlockType b) noexcept
{
    return isLeaf(b) || isPlant(b) || isLadder(b) ||
           b == BlockType::RedstoneWire ||
           b == BlockType::RedstoneTorch || b == BlockType::Lever ||
           b == BlockType::Repeater;
}
[[nodiscard]] constexpr bool isTranslucent(BlockType b) noexcept
{
    return isLiquid(b) || b == BlockType::Glass || b == BlockType::Ice;
}
[[nodiscard]] constexpr bool isTransparent(BlockType b) noexcept { return isCutout(b) || isTranslucent(b); }
[[nodiscard]] constexpr bool isOpaque(BlockType b) noexcept { return !isAir(b) && !isTransparent(b); }
[[nodiscard]] constexpr bool isSolid(BlockType b) noexcept
{
    return !isAir(b) && !isLiquid(b) && !isPlant(b) && !isLadder(b) &&
           b != BlockType::RedstoneWire && b != BlockType::RedstoneTorch &&
           b != BlockType::Lever && b != BlockType::Repeater;
}
