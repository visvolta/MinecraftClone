#include "content/MinecraftContent.h"

#include "content/ContentCatalog.h"

#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace mc::content
{
namespace
{
core::ResourceLocation id(std::string_view path)
{
    return core::ResourceLocation("minecraft", path);
}

core::ResourceLocation texture(std::string_view name)
{
    return core::ResourceLocation("minecraft", std::string("blocks/") + std::string(name));
}

BlockTextures cube(std::string_view name)
{
    BlockTextures result;
    result.all = texture(name);
    return result;
}

BlockTextures column(std::string_view side, std::string_view end)
{
    BlockTextures result;
    result.side = texture(side);
    result.top = texture(end);
    result.bottom = texture(end);
    return result;
}

BlockTextures topSideBottom(
    std::string_view side,
    std::string_view top,
    std::string_view bottom)
{
    BlockTextures result;
    result.side = texture(side);
    result.top = texture(top);
    result.bottom = texture(bottom);
    return result;
}

BlockTextures oriented(
    std::string_view side,
    std::string_view top,
    std::string_view front)
{
    BlockTextures result;
    result.side = texture(side);
    result.top = texture(top);
    result.bottom = texture(top);
    result.front = texture(front);
    result.horizontalFacing = true;
    return result;
}

BlockStateSchema stateSchemaFor(BlockType type)
{
    BlockStateSchema schema;
    if (isLiquid(type))
    {
        schema.properties.push_back({"level", 0x0FU, 0U, 15U, {}});
    }
    else if (isFurnace(type) || type == BlockType::Chest)
    {
        schema.defaultProperties = 3U;
        schema.properties.push_back({
            "facing",
            0x07U,
            2U,
            5U,
            {"north", "south", "west", "east"}
        });
    }
    else if (type == BlockType::Farmland)
    {
        schema.properties.push_back({"moisture", 0x07U, 0U, 7U, {}});
    }
    else if (type == BlockType::Wheat)
    {
        schema.properties.push_back({"age", 0x07U, 0U, 7U, {}});
    }
    else if (type == BlockType::Carrots || type == BlockType::Potatoes)
    {
        schema.properties.push_back({"age", 0x07U, 0U, 7U, {}});
    }
    else if (type == BlockType::Beetroots)
    {
        schema.properties.push_back({"age", 0x03U, 0U, 3U, {}});
    }
    else if (type == BlockType::RedstoneWire)
    {
        schema.properties.push_back({"power", 0x0FU, 0U, 15U, {}});
    }
    else if (type == BlockType::Lever || type == BlockType::Repeater ||
             type == BlockType::RedstoneTorch)
    {
        schema.defaultProperties = type == BlockType::RedstoneTorch ? 1U : 0U;
        schema.properties.push_back({
            "powered", 0x01U, 0U, 1U, {"false", "true"}
        });
    }
    return schema;
}

BlockBehaviour behaviourFor(BlockType type)
{
    BlockBehaviour behaviour;
    behaviour.breaking = getBlockProperties(type);
    behaviour.shape = &getBlockShape(type);
    behaviour.traits = {
        isLiquid(type),
        isLeaf(type),
        isPlant(type),
        isLadder(type),
        isCutout(type),
        isTranslucent(type),
        isOpaque(type),
        isSolid(type)
    };

    if (type == BlockType::Grass || type == BlockType::TallGrass)
        behaviour.tint = BlockTint::Grass;
    else if (type == BlockType::OakLeaves ||
             type == BlockType::JungleLeaves ||
             type == BlockType::AcaciaLeaves ||
             type == BlockType::DarkOakLeaves)
        behaviour.tint = BlockTint::Foliage;
    else if (type == BlockType::SpruceLeaves)
        behaviour.tint = BlockTint::SpruceFoliage;
    else if (type == BlockType::BirchLeaves)
        behaviour.tint = BlockTint::BirchFoliage;

    if (type == BlockType::Water || type == BlockType::Ice)
        behaviour.lightOpacity = 3;
    else if (isLeaf(type))
        behaviour.lightOpacity = 1;
    else if (type == BlockType::Air || isPlant(type) || isLadder(type) ||
        type == BlockType::Lava || type == BlockType::Glass ||
        type == BlockType::RedstoneWire ||
        type == BlockType::RedstoneTorch || type == BlockType::Lever ||
        type == BlockType::Repeater)
        behaviour.lightOpacity = 0;
    else
        behaviour.lightOpacity = 15;
    behaviour.lightEmission =
        (type == BlockType::Lava || type == BlockType::Glowstone)
            ? 15U
            : (type == BlockType::LitFurnace
                ? 13U
                : (type == BlockType::RedstoneTorch ? 7U : 0U));

    switch (type)
    {
        case BlockType::Air:
        case BlockType::Water:
        case BlockType::Lava:
        case BlockType::Bedrock:
        case BlockType::Spawner:
            behaviour.dropRule = BlockDropRule::None;
            break;
        case BlockType::Stone: behaviour.dropRule = BlockDropRule::Cobblestone; break;
        case BlockType::Grass: behaviour.dropRule = BlockDropRule::Dirt; break;
        case BlockType::Gravel: behaviour.dropRule = BlockDropRule::FlintOrGravel; break;
        case BlockType::Clay: behaviour.dropRule = BlockDropRule::ClayBalls; break;
        case BlockType::CoalOre: behaviour.dropRule = BlockDropRule::Coal; break;
        case BlockType::DiamondOre: behaviour.dropRule = BlockDropRule::Diamond; break;
        case BlockType::RedstoneOre: behaviour.dropRule = BlockDropRule::Redstone; break;
        case BlockType::LapisOre: behaviour.dropRule = BlockDropRule::Lapis; break;
        case BlockType::TallGrass: behaviour.dropRule = BlockDropRule::Seeds; break;
        case BlockType::OakLeaves: behaviour.dropRule = BlockDropRule::OakSapling; break;
        case BlockType::SpruceLeaves: behaviour.dropRule = BlockDropRule::SpruceSapling; break;
        case BlockType::BirchLeaves: behaviour.dropRule = BlockDropRule::BirchSapling; break;
        case BlockType::LitFurnace: behaviour.dropRule = BlockDropRule::Furnace; break;
        default: behaviour.dropRule = BlockDropRule::Self; break;
    }
    const auto lootName = [](BlockDropRule rule) -> std::string_view
    {
        switch (rule)
        {
            case BlockDropRule::None: return "none";
            case BlockDropRule::Self: return "self";
            case BlockDropRule::Cobblestone: return "cobblestone";
            case BlockDropRule::Dirt: return "dirt";
            case BlockDropRule::FlintOrGravel: return "flint_or_gravel";
            case BlockDropRule::ClayBalls: return "clay_balls";
            case BlockDropRule::Coal: return "coal";
            case BlockDropRule::Diamond: return "diamond";
            case BlockDropRule::Redstone: return "redstone";
            case BlockDropRule::Lapis: return "lapis";
            case BlockDropRule::Seeds: return "seeds";
            case BlockDropRule::OakSapling: return "oak_sapling";
            case BlockDropRule::SpruceSapling: return "spruce_sapling";
            case BlockDropRule::BirchSapling: return "birch_sapling";
            case BlockDropRule::Furnace: return "furnace";
        }
        return "self";
    };
    behaviour.lootTable = id(lootName(behaviour.dropRule));
    return behaviour;
}

void addBlock(
    ContentCatalog& catalog,
    BlockType legacyType,
    std::string_view registryPath,
    std::string_view displayName,
    BlockTextures textures = {})
{
    const core::ResourceLocation name = id(registryPath);
    const RenderLayer renderLayer = isLiquid(legacyType) ||
            isTranslucent(legacyType)
        ? RenderLayer::Translucent
        : (isLeaf(legacyType)
            ? RenderLayer::CutoutMipped
            : (isCutout(legacyType)
                ? RenderLayer::Cutout
                : RenderLayer::Solid));
    catalog.registerBlock(
        name,
        {
            legacyType,
            std::string(displayName),
            std::move(textures),
            stateSchemaFor(legacyType),
            behaviourFor(legacyType),
            renderLayer
        }
    );
    if (legacyType != BlockType::Air)
    {
        catalog.registerItem(
            name,
            {
                itemFromBlock(legacyType),
                std::string(displayName),
                name,
                getItemProperties(itemFromBlock(legacyType))
            }
        );
    }
}

void addItem(
    ContentCatalog& catalog,
    ItemType legacyType,
    std::string_view registryPath,
    std::string_view displayName)
{
    catalog.registerItem(
        id(registryPath),
        {
            legacyType,
            std::string(displayName),
            std::nullopt,
            getItemProperties(legacyType)
        }
    );
}
}

core::ResourceLocation MinecraftContentModule::id() const
{
    return core::ResourceLocation("minecraft:builtins");
}

void MinecraftContentModule::registerContent(ContentCatalog& catalog)
{
    registerMinecraftContent(catalog);
}

void registerMinecraftContent(ContentCatalog& catalog)
{
    const auto addLoot = [&catalog](
        std::string_view name,
        BlockDropRule rule)
    {
        catalog.registerLootTable(id(name), {rule});
    };
    addLoot("none", BlockDropRule::None);
    addLoot("self", BlockDropRule::Self);
    addLoot("cobblestone", BlockDropRule::Cobblestone);
    addLoot("dirt", BlockDropRule::Dirt);
    addLoot("flint_or_gravel", BlockDropRule::FlintOrGravel);
    addLoot("clay_balls", BlockDropRule::ClayBalls);
    addLoot("coal", BlockDropRule::Coal);
    addLoot("diamond", BlockDropRule::Diamond);
    addLoot("redstone", BlockDropRule::Redstone);
    addLoot("lapis", BlockDropRule::Lapis);
    addLoot("seeds", BlockDropRule::Seeds);
    addLoot("oak_sapling", BlockDropRule::OakSapling);
    addLoot("spruce_sapling", BlockDropRule::SpruceSapling);
    addLoot("birch_sapling", BlockDropRule::BirchSapling);
    addLoot("furnace", BlockDropRule::Furnace);

    addBlock(catalog, BlockType::Air, "air", "Air");
    addBlock(catalog, BlockType::Dirt, "dirt", "Dirt", cube("dirt"));

    BlockTextures grass = topSideBottom("grass_side", "grass_top", "dirt");
    grass.sideOverlay = texture("grass_side_overlay");
    addBlock(catalog, BlockType::Grass, "grass", "Grass Block", std::move(grass));

    addBlock(catalog, BlockType::Stone, "stone", "Stone", cube("stone"));
    addBlock(catalog, BlockType::Cobblestone, "cobblestone", "Cobblestone", cube("cobblestone"));
    addBlock(catalog, BlockType::Gravel, "gravel", "Gravel", cube("gravel"));
    addBlock(catalog, BlockType::Water, "water", "Water");
    addBlock(catalog, BlockType::Bedrock, "bedrock", "Bedrock", cube("bedrock"));
    addBlock(catalog, BlockType::OakLog, "oak_log", "Oak Wood", column("log_oak", "log_oak_top"));
    addBlock(catalog, BlockType::OakLeaves, "oak_leaves", "Oak Leaves", cube("leaves_oak"));
    addBlock(catalog, BlockType::Sand, "sand", "Sand", cube("sand"));
    addBlock(catalog, BlockType::Clay, "clay", "Clay", cube("clay"));
    addBlock(catalog, BlockType::IronOre, "iron_ore", "Iron Ore", cube("iron_ore"));
    addBlock(catalog, BlockType::GoldOre, "gold_ore", "Gold Ore", cube("gold_ore"));
    addBlock(catalog, BlockType::RedstoneOre, "redstone_ore", "Redstone Ore", cube("redstone_ore"));
    addBlock(catalog, BlockType::DiamondOre, "diamond_ore", "Diamond Ore", cube("diamond_ore"));
    addBlock(catalog, BlockType::CoalOre, "coal_ore", "Coal Ore", cube("coal_ore"));
    addBlock(catalog, BlockType::SpruceLeaves, "spruce_leaves", "Spruce Leaves", cube("leaves_spruce"));
    addBlock(catalog, BlockType::BirchLeaves, "birch_leaves", "Birch Leaves", cube("leaves_birch"));
    addBlock(catalog, BlockType::SpruceLog, "spruce_log", "Spruce Wood", column("log_spruce", "log_spruce_top"));
    addBlock(catalog, BlockType::BirchLog, "birch_log", "Birch Wood", column("log_birch", "log_birch_top"));
    addBlock(catalog, BlockType::BrownMushroom, "brown_mushroom", "Brown Mushroom", cube("mushroom_brown"));
    addBlock(catalog, BlockType::RedMushroom, "red_mushroom", "Red Mushroom", cube("mushroom_red"));
    addBlock(catalog, BlockType::TallGrass, "tallgrass", "Tall Grass", cube("tallgrass"));
    addBlock(catalog, BlockType::Rose, "red_flower", "Rose", cube("flower_rose"));
    addBlock(catalog, BlockType::Dandelion, "yellow_flower", "Dandelion", cube("flower_dandelion"));
    addBlock(catalog, BlockType::MossyCobblestone, "mossy_cobblestone", "Moss Stone", cube("cobblestone_mossy"));
    addBlock(catalog, BlockType::Spawner, "mob_spawner", "Monster Spawner", cube("mob_spawner"));
    addBlock(catalog, BlockType::Chest, "chest", "Chest", oriented("legacy_chest_side", "legacy_chest_top", "legacy_chest_front"));
    addBlock(catalog, BlockType::Pumpkin, "pumpkin", "Pumpkin", column("pumpkin_side", "pumpkin_top"));

    BlockTextures crafting = topSideBottom("crafting_table_side", "crafting_table_top", "planks_oak");
    crafting.front = texture("crafting_table_front");
    crafting.right = texture("crafting_table_front");
    addBlock(catalog, BlockType::CraftingTable, "crafting_table", "Crafting Table", std::move(crafting));

    addBlock(catalog, BlockType::OakPlanks, "oak_planks", "Oak Wood Planks", cube("planks_oak"));
    addBlock(catalog, BlockType::SprucePlanks, "spruce_planks", "Spruce Wood Planks", cube("planks_spruce"));
    addBlock(catalog, BlockType::BirchPlanks, "birch_planks", "Birch Wood Planks", cube("planks_birch"));
    addBlock(catalog, BlockType::Sandstone, "sandstone", "Sandstone", topSideBottom("sandstone_normal", "sandstone_top", "sandstone_bottom"));
    addBlock(catalog, BlockType::Bricks, "brick_block", "Bricks", cube("brick"));
    addBlock(catalog, BlockType::HayBale, "hay_block", "Hay Bale", column("hay_block_side", "hay_block_top"));
    addBlock(catalog, BlockType::Ladder, "ladder", "Ladder", cube("ladder"));
    addBlock(catalog, BlockType::LapisBlock, "lapis_block", "Lapis Lazuli Block", cube("lapis_block"));
    addBlock(catalog, BlockType::LapisOre, "lapis_ore", "Lapis Lazuli Ore", cube("lapis_ore"));
    addBlock(catalog, BlockType::IronBlock, "iron_block", "Block of Iron", cube("iron_block"));
    addBlock(catalog, BlockType::GoldBlock, "gold_block", "Block of Gold", cube("gold_block"));
    addBlock(catalog, BlockType::WhiteWool, "white_wool", "White Wool", cube("wool_colored_white"));
    addBlock(catalog, BlockType::OrangeWool, "orange_wool", "Orange Wool", cube("wool_colored_orange"));
    addBlock(catalog, BlockType::MagentaWool, "magenta_wool", "Magenta Wool", cube("wool_colored_magenta"));
    addBlock(catalog, BlockType::LightBlueWool, "light_blue_wool", "Light Blue Wool", cube("wool_colored_light_blue"));
    addBlock(catalog, BlockType::YellowWool, "yellow_wool", "Yellow Wool", cube("wool_colored_yellow"));
    addBlock(catalog, BlockType::LimeWool, "lime_wool", "Lime Wool", cube("wool_colored_lime"));
    addBlock(catalog, BlockType::PinkWool, "pink_wool", "Pink Wool", cube("wool_colored_pink"));
    addBlock(catalog, BlockType::GrayWool, "gray_wool", "Gray Wool", cube("wool_colored_gray"));
    addBlock(catalog, BlockType::LightGrayWool, "light_gray_wool", "Light Gray Wool", cube("wool_colored_silver"));
    addBlock(catalog, BlockType::CyanWool, "cyan_wool", "Cyan Wool", cube("wool_colored_cyan"));
    addBlock(catalog, BlockType::PurpleWool, "purple_wool", "Purple Wool", cube("wool_colored_purple"));
    addBlock(catalog, BlockType::BlueWool, "blue_wool", "Blue Wool", cube("wool_colored_blue"));
    addBlock(catalog, BlockType::BrownWool, "brown_wool", "Brown Wool", cube("wool_colored_brown"));
    addBlock(catalog, BlockType::GreenWool, "green_wool", "Green Wool", cube("wool_colored_green"));
    addBlock(catalog, BlockType::RedWool, "red_wool", "Red Wool", cube("wool_colored_red"));
    addBlock(catalog, BlockType::BlackWool, "black_wool", "Black Wool", cube("wool_colored_black"));
    addBlock(catalog, BlockType::Obsidian, "obsidian", "Obsidian", cube("obsidian"));
    addBlock(catalog, BlockType::Furnace, "furnace", "Furnace", oriented("furnace_side", "furnace_top", "furnace_front_off"));
    addBlock(catalog, BlockType::LitFurnace, "lit_furnace", "Burning Furnace", oriented("furnace_side", "furnace_top", "furnace_front_on"));
    addBlock(catalog, BlockType::Lava, "lava", "Lava");

    addBlock(catalog, BlockType::JungleLog, "jungle_log", "Jungle Wood", column("log_jungle", "log_jungle_top"));
    addBlock(catalog, BlockType::JungleLeaves, "jungle_leaves", "Jungle Leaves", cube("leaves_jungle"));
    addBlock(catalog, BlockType::AcaciaLog, "acacia_log", "Acacia Wood", column("log_acacia", "log_acacia_top"));
    addBlock(catalog, BlockType::AcaciaLeaves, "acacia_leaves", "Acacia Leaves", cube("leaves_acacia"));
    addBlock(catalog, BlockType::DarkOakLog, "dark_oak_log", "Dark Oak Wood", column("log_big_oak", "log_big_oak_top"));
    addBlock(catalog, BlockType::DarkOakLeaves, "dark_oak_leaves", "Dark Oak Leaves", cube("leaves_big_oak"));
    addBlock(catalog, BlockType::JunglePlanks, "jungle_planks", "Jungle Wood Planks", cube("planks_jungle"));
    addBlock(catalog, BlockType::AcaciaPlanks, "acacia_planks", "Acacia Wood Planks", cube("planks_acacia"));
    addBlock(catalog, BlockType::DarkOakPlanks, "dark_oak_planks", "Dark Oak Wood Planks", cube("planks_big_oak"));
    addBlock(catalog, BlockType::Podzol, "podzol", "Podzol", topSideBottom("dirt_podzol_side", "dirt_podzol_top", "dirt"));
    addBlock(catalog, BlockType::Mycelium, "mycelium", "Mycelium", topSideBottom("mycelium_side", "mycelium_top", "dirt"));
    addBlock(catalog, BlockType::Snow, "snow", "Snow", cube("snow"));
    addBlock(catalog, BlockType::Ice, "ice", "Ice", cube("ice"));
    addBlock(catalog, BlockType::Cactus, "cactus", "Cactus", topSideBottom("cactus_side", "cactus_top", "cactus_bottom"));
    addBlock(catalog, BlockType::SugarCane, "reeds", "Sugar Canes", cube("reeds"));
    addBlock(catalog, BlockType::Farmland, "farmland", "Farmland", topSideBottom("dirt", "farmland_dry", "dirt"));
    addBlock(catalog, BlockType::Wheat, "wheat", "Wheat Crops", cube("wheat_stage_0"));
    addBlock(catalog, BlockType::Carrots, "carrots", "Carrots", cube("carrots_stage_0"));
    addBlock(catalog, BlockType::Potatoes, "potatoes", "Potatoes", cube("potatoes_stage_0"));
    addBlock(catalog, BlockType::Beetroots, "beetroots", "Beetroots", cube("beetroots_stage_0"));
    addBlock(catalog, BlockType::Glass, "glass", "Glass", cube("glass"));
    addBlock(catalog, BlockType::Netherrack, "netherrack", "Netherrack", cube("netherrack"));
    addBlock(catalog, BlockType::SoulSand, "soul_sand", "Soul Sand", cube("soul_sand"));
    addBlock(catalog, BlockType::NetherBricks, "nether_brick", "Nether Bricks", cube("nether_brick"));
    addBlock(catalog, BlockType::Glowstone, "glowstone", "Glowstone", cube("glowstone"));
    addBlock(catalog, BlockType::EndStone, "end_stone", "End Stone", cube("end_stone"));
    addBlock(catalog, BlockType::RedstoneWire, "redstone_wire", "Redstone Wire", cube("redstone_dust_dot"));
    addBlock(catalog, BlockType::RedstoneTorch, "redstone_torch", "Redstone Torch", cube("redstone_torch_on"));
    addBlock(catalog, BlockType::Lever, "lever", "Lever", cube("lever"));
    addBlock(catalog, BlockType::Repeater, "unpowered_repeater", "Redstone Repeater", cube("repeater_off"));
    addBlock(catalog, BlockType::Piston, "piston", "Piston", topSideBottom("piston_side", "piston_top_normal", "piston_bottom"));
    addBlock(catalog, BlockType::StickyPiston, "sticky_piston", "Sticky Piston", topSideBottom("piston_side", "piston_top_sticky", "piston_bottom"));
    addBlock(catalog, BlockType::TNT, "tnt", "TNT", topSideBottom("tnt_side", "tnt_top", "tnt_bottom"));

    addItem(catalog, ItemType::Stick, "stick", "Stick");
    addItem(catalog, ItemType::Coal, "coal", "Coal");
    addItem(catalog, ItemType::Diamond, "diamond", "Diamond");
    addItem(catalog, ItemType::IronIngot, "iron_ingot", "Iron Ingot");
    addItem(catalog, ItemType::GoldIngot, "gold_ingot", "Gold Ingot");
    addItem(catalog, ItemType::Flint, "flint", "Flint");
    addItem(catalog, ItemType::RedstoneDust, "redstone", "Redstone");
    addItem(catalog, ItemType::ClayBall, "clay_ball", "Clay");
    addItem(catalog, ItemType::LapisLazuli, "dye", "Lapis Lazuli");
    addItem(catalog, ItemType::Seeds, "wheat_seeds", "Seeds");
    addItem(catalog, ItemType::OakSapling, "oak_sapling", "Oak Sapling");
    addItem(catalog, ItemType::SpruceSapling, "spruce_sapling", "Spruce Sapling");
    addItem(catalog, ItemType::BirchSapling, "birch_sapling", "Birch Sapling");
    addItem(catalog, ItemType::Charcoal, "charcoal", "Charcoal");
    addItem(catalog, ItemType::Brick, "brick", "Brick");
    addItem(catalog, ItemType::WoodenShovel, "wooden_shovel", "Wooden Shovel");
    addItem(catalog, ItemType::WoodenPickaxe, "wooden_pickaxe", "Wooden Pickaxe");
    addItem(catalog, ItemType::WoodenAxe, "wooden_axe", "Wooden Axe");
    addItem(catalog, ItemType::StoneShovel, "stone_shovel", "Stone Shovel");
    addItem(catalog, ItemType::StonePickaxe, "stone_pickaxe", "Stone Pickaxe");
    addItem(catalog, ItemType::StoneAxe, "stone_axe", "Stone Axe");
    addItem(catalog, ItemType::IronShovel, "iron_shovel", "Iron Shovel");
    addItem(catalog, ItemType::IronPickaxe, "iron_pickaxe", "Iron Pickaxe");
    addItem(catalog, ItemType::IronAxe, "iron_axe", "Iron Axe");
    addItem(catalog, ItemType::DiamondShovel, "diamond_shovel", "Diamond Shovel");
    addItem(catalog, ItemType::DiamondPickaxe, "diamond_pickaxe", "Diamond Pickaxe");
    addItem(catalog, ItemType::DiamondAxe, "diamond_axe", "Diamond Axe");
    addItem(catalog, ItemType::GoldenShovel, "golden_shovel", "Golden Shovel");
    addItem(catalog, ItemType::GoldenPickaxe, "golden_pickaxe", "Golden Pickaxe");
    addItem(catalog, ItemType::GoldenAxe, "golden_axe", "Golden Axe");
    addItem(catalog, ItemType::Apple, "apple", "Apple");
    addItem(catalog, ItemType::Bread, "bread", "Bread");
    addItem(catalog, ItemType::Carrot, "carrot", "Carrot");
    addItem(catalog, ItemType::Potato, "potato", "Potato");
    addItem(catalog, ItemType::BakedPotato, "baked_potato", "Baked Potato");
    addItem(catalog, ItemType::CookedBeef, "cooked_beef", "Steak");
    addItem(catalog, ItemType::Shield, "shield", "Shield");
    addItem(catalog, ItemType::IronHelmet, "iron_helmet", "Iron Helmet");
    addItem(catalog, ItemType::IronChestplate, "iron_chestplate", "Iron Chestplate");
    addItem(catalog, ItemType::IronLeggings, "iron_leggings", "Iron Leggings");
    addItem(catalog, ItemType::IronBoots, "iron_boots", "Iron Boots");
    addItem(catalog, ItemType::DiamondHelmet, "diamond_helmet", "Diamond Helmet");
    addItem(catalog, ItemType::DiamondChestplate, "diamond_chestplate", "Diamond Chestplate");
    addItem(catalog, ItemType::DiamondLeggings, "diamond_leggings", "Diamond Leggings");
    addItem(catalog, ItemType::DiamondBoots, "diamond_boots", "Diamond Boots");

    catalog.registerEntityType(id("item"), {"Dropped Item"});
    catalog.registerBlockEntityType(id("furnace"), {"Furnace", 1});
    catalog.registerBlockEntityType(id("chest"), {"Chest", 1});
}
}
