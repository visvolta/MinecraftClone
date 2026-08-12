#include "content/MinecraftContent.h"

#include "content/ContentCatalog.h"
#include "content/resources/ResourcePack.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <unordered_map>

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

    if (type == BlockType::Grass || type == BlockType::TallGrass ||
        type == BlockType::Fern)
        behaviour.tint = BlockTint::Grass;
    else if (type == BlockType::Vine)
        behaviour.tint = BlockTint::Foliage;
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
        case BlockType::JungleLeaves: behaviour.dropRule = BlockDropRule::JungleSapling; break;
        case BlockType::AcaciaLeaves: behaviour.dropRule = BlockDropRule::AcaciaSapling; break;
        case BlockType::DarkOakLeaves: behaviour.dropRule = BlockDropRule::DarkOakSapling; break;
        case BlockType::LitFurnace: behaviour.dropRule = BlockDropRule::Furnace; break;
        case BlockType::Farmland: behaviour.dropRule = BlockDropRule::Farmland; break;
        case BlockType::Glass:
        case BlockType::Ice:
        case BlockType::Vine:
            behaviour.dropRule = BlockDropRule::GlassLike;
            break;
        case BlockType::Snow: behaviour.dropRule = BlockDropRule::Snowball; break;
        case BlockType::Glowstone: behaviour.dropRule = BlockDropRule::GlowstoneDust; break;
        case BlockType::RedstoneWire: behaviour.dropRule = BlockDropRule::RedstoneDust; break;
        case BlockType::Wheat: behaviour.dropRule = BlockDropRule::WheatCrop; break;
        case BlockType::Carrots: behaviour.dropRule = BlockDropRule::CarrotCrop; break;
        case BlockType::Potatoes: behaviour.dropRule = BlockDropRule::PotatoCrop; break;
        case BlockType::Beetroots: behaviour.dropRule = BlockDropRule::BeetrootCrop; break;
        case BlockType::Melon: behaviour.dropRule = BlockDropRule::MelonSlices; break;
        case BlockType::Cocoa: behaviour.dropRule = BlockDropRule::CocoaBeans; break;
        case BlockType::Bookshelf: behaviour.dropRule = BlockDropRule::Books; break;
        case BlockType::Cobweb: behaviour.dropRule = BlockDropRule::CobwebString; break;
        case BlockType::DeadBush: behaviour.dropRule = BlockDropRule::DeadBushSticks; break;
        case BlockType::BrownMushroomBlock:
        case BlockType::RedMushroomBlock:
            behaviour.dropRule = BlockDropRule::MushroomCap;
            break;
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
            case BlockDropRule::JungleSapling: return "jungle_sapling";
            case BlockDropRule::AcaciaSapling: return "acacia_sapling";
            case BlockDropRule::DarkOakSapling: return "dark_oak_sapling";
            case BlockDropRule::Furnace: return "furnace";
            case BlockDropRule::Farmland: return "farmland";
            case BlockDropRule::GlassLike: return "glass_like";
            case BlockDropRule::Snowball: return "snowball";
            case BlockDropRule::GlowstoneDust: return "glowstone_dust";
            case BlockDropRule::RedstoneDust: return "redstone_dust";
            case BlockDropRule::WheatCrop: return "wheat_crop";
            case BlockDropRule::CarrotCrop: return "carrot_crop";
            case BlockDropRule::PotatoCrop: return "potato_crop";
            case BlockDropRule::BeetrootCrop: return "beetroot_crop";
            case BlockDropRule::MelonSlices: return "melon_slices";
            case BlockDropRule::CocoaBeans: return "cocoa_beans";
            case BlockDropRule::Books: return "books";
            case BlockDropRule::CobwebString: return "cobweb_string";
            case BlockDropRule::DeadBushSticks: return "dead_bush_sticks";
            case BlockDropRule::MushroomCap: return "mushroom_cap";
        }
        return "self";
    };
    behaviour.lootTable = id(lootName(behaviour.dropRule));

    behaviour.replaceable =
        type == BlockType::Air || isLiquid(type) || isPlant(type) ||
        type == BlockType::Snow || type == BlockType::Vine;
    behaviour.gravityAffected =
        type == BlockType::Sand || type == BlockType::Gravel;
    behaviour.randomTicks =
        type == BlockType::Grass || type == BlockType::Mycelium ||
        isLeaf(type) || isCrop(type) || type == BlockType::Cactus ||
        type == BlockType::SugarCane || type == BlockType::Ice ||
        type == BlockType::Snow;
    behaviour.requiresSupport =
        isPlant(type) || isLadder(type) ||
        type == BlockType::RedstoneWire ||
        type == BlockType::RedstoneTorch ||
        type == BlockType::Lever || type == BlockType::Repeater;

    if (type == BlockType::Lava)
    {
        behaviour.traits.translucent = false;
        behaviour.traits.opaque = false;
    }

    if (isFurnace(type))
        behaviour.blockEntityType = id("furnace");
    else if (type == BlockType::Chest)
        behaviour.blockEntityType = id("chest");
    else if (type == BlockType::Spawner)
        behaviour.blockEntityType = id("mob_spawner");

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
    const RenderLayer renderLayer =
        legacyType == BlockType::Water ||
        legacyType == BlockType::Glass ||
        legacyType == BlockType::Ice
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
    if (legacyType != BlockType::Air && !isCrop(legacyType))
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

std::string displayNameFor(std::string path)
{
    bool capitalize = true;
    for (char& character : path)
    {
        if (character == '_' || character == '/')
        {
            character = ' ';
            capitalize = true;
        }
        else if (capitalize)
        {
            character = static_cast<char>(std::toupper(
                static_cast<unsigned char>(character)
            ));
            capitalize = false;
        }
    }
    return path;
}

bool containsAny(std::string_view value, std::initializer_list<std::string_view> terms)
{
    return std::any_of(
        terms.begin(), terms.end(),
        [value](std::string_view term) { return value.find(term) != std::string_view::npos; }
    );
}

bool endsWith(std::string_view value, std::string_view suffix)
{
    return value.size() >= suffix.size() &&
           value.substr(value.size() - suffix.size()) == suffix;
}

bool equalsAny(std::string_view value, std::initializer_list<std::string_view> terms)
{
    return std::find(terms.begin(), terms.end(), value) != terms.end();
}

std::optional<core::ResourceLocation> blockEntityTypeForBlock(
    std::string_view name)
{
    if (name == "furnace" || name == "lit_furnace")
        return id("furnace");
    if (name == "chest" || name == "trapped_chest")
        return id("chest");
    if (name == "ender_chest") return id("ender_chest");
    if (name == "jukebox") return id("jukebox");
    if (name == "dispenser") return id("dispenser");
    if (name == "dropper") return id("dropper");
    if (name == "standing_sign" || name == "wall_sign")
        return id("sign");
    if (name == "mob_spawner") return id("mob_spawner");
    if (name == "noteblock") return id("noteblock");
    if (name == "piston_extension") return id("piston");
    if (name == "brewing_stand") return id("brewing_stand");
    if (name == "enchanting_table") return id("enchanting_table");
    if (name == "end_portal") return id("end_portal");
    if (name == "beacon") return id("beacon");
    if (name == "skull") return id("skull");
    if (name == "daylight_detector" ||
        name == "daylight_detector_inverted")
        return id("daylight_detector");
    if (name == "hopper") return id("hopper");
    if (name == "unpowered_comparator" ||
        name == "powered_comparator")
        return id("comparator");
    if (name == "flower_pot") return id("flower_pot");
    if (name == "standing_banner" || name == "wall_banner")
        return id("banner");
    if (name == "structure_block") return id("structure_block");
    if (name == "end_gateway") return id("end_gateway");
    if (name == "command_block" ||
        name == "chain_command_block" ||
        name == "repeating_command_block")
        return id("command_block");
    if (endsWith(name, "_shulker_box"))
        return id("shulker_box");
    if (name == "bed") return id("bed");
    return std::nullopt;
}

BlockBehaviour resourceBehaviour(std::string_view name)
{
    BlockBehaviour behaviour;
    behaviour.breaking = getBlockProperties(BlockType::Stone);
    behaviour.shape = &getBlockShape(BlockType::Stone);
    behaviour.lootTable = id("self");
    behaviour.dropRule = BlockDropRule::Self;
    behaviour.traits.opaque = true;
    behaviour.traits.solid = true;
    behaviour.lightOpacity = 15;

    // Keep these classifications token-aware. Substring matching "air" used
    // to classify every *_stairs block as a plant, removing its collision and
    // putting it in the cutout render pass.
    const bool plant = equalsAny(name, {
        "air", "tallgrass", "double_plant", "deadbush", "yellow_flower",
        "red_flower", "brown_mushroom", "red_mushroom", "wheat",
        "carrots", "potatoes", "beetroots", "reeds", "vine",
        "waterlily", "nether_wart", "cocoa", "chorus_plant",
        "chorus_flower",
        "poppy", "blue_orchid", "allium", "houstonia",
        "red_tulip", "orange_tulip", "white_tulip", "pink_tulip",
        "oxeye_daisy", "sunflower", "syringa", "double_grass",
        "double_fern", "double_rose", "paeonia"
    }) || endsWith(name, "_sapling") || endsWith(name, "_stem");
    const bool rail = name == "rail" || endsWith(name, "_rail");
    const bool component = containsAny(name, {
        "torch", "button", "lever", "pressure_plate", "tripwire"
    }) || name == "redstone_wire" || equalsAny(name, {
        "standing_sign", "wall_sign", "standing_banner", "wall_banner",
        "structure_void"
    });
    const bool portal = equalsAny(name, {
        "portal", "end_portal", "end_gateway", "fire"
    });
    const bool leaves = name == "leaves" || name == "leaves2" ||
        endsWith(name, "_leaves");
    const bool pane = name == "glass_pane" || name == "iron_bars" ||
        endsWith(name, "_stained_glass_pane");
    const bool fence = name == "fence" || endsWith(name, "_fence") ||
        endsWith(name, "_fence_gate");
    const bool doubleSlab = endsWith(name, "_double_slab");
    const bool partialSolid = endsWith(name, "_stairs") ||
        (endsWith(name, "_slab") && !doubleSlab) || endsWith(name, "_door") ||
        endsWith(name, "_trapdoor") || endsWith(name, "_wall") ||
        endsWith(name, "_carpet") || fence || pane ||
        equalsAny(name, {
            "stone_slab", "wooden_slab", "snow_layer", "bed", "cake",
            "cauldron", "hopper", "anvil", "brewing_stand", "flower_pot",
            "enchanting_table", "daylight_detector", "daylight_detector_inverted",
            "trapped_chest", "ender_chest", "skull"
        });

    if (plant || rail || component || portal)
    {
        behaviour.shape = &getBlockShape(BlockType::TallGrass);
        behaviour.traits = {};
        behaviour.traits.plant = plant;
        behaviour.traits.cutout = true;
        behaviour.lightOpacity = 0;
    }
    else if (partialSolid)
    {
        behaviour.traits.opaque = false;
        behaviour.traits.solid = true;
        behaviour.lightOpacity = 0;
        behaviour.traits.cutout = pane || endsWith(name, "_door") ||
            endsWith(name, "_trapdoor");
        if (endsWith(name, "_chest"))
            behaviour.shape = &getBlockShape(BlockType::Chest);
        else if (name == "skull" || name == "flower_pot")
            behaviour.shape = &getBlockShape(BlockType::Lever);
    }

    if (containsAny(name, {"glass", "ice"}) || portal)
    {
        behaviour.shape = &getBlockShape(BlockType::Glass);
        behaviour.traits.opaque = false;
        behaviour.traits.translucent = true;
        behaviour.traits.solid = !portal;
        behaviour.traits.cutout = false;
        behaviour.lightOpacity = name.find("ice") != std::string_view::npos ? 3 : 0;
    }
    if (leaves)
    {
        behaviour.shape = &getBlockShape(BlockType::OakLeaves);
        behaviour.traits = {};
        behaviour.traits.leaf = true;
        behaviour.traits.cutout = true;
        behaviour.lightOpacity = 1;
        behaviour.tint = name.find("spruce") != std::string_view::npos
            ? BlockTint::SpruceFoliage
            : (name.find("birch") != std::string_view::npos
                ? BlockTint::BirchFoliage : BlockTint::Foliage);
    }
    if (name == "grass" || name == "grass_block" ||
        name.find("tallgrass") != std::string_view::npos ||
        name == "double_grass" || name == "double_fern")
        behaviour.tint = BlockTint::Grass;
    else if (name == "vine")
        behaviour.tint = BlockTint::Foliage;

    if (containsAny(name, {
        "planks", "log", "wood", "fence", "door", "bookshelf",
        "crafting_table", "chest"
    }))
        behaviour.breaking = getBlockProperties(BlockType::OakPlanks);
    else if (leaves)
        behaviour.breaking = getBlockProperties(BlockType::OakLeaves);
    else if (plant || rail || component)
        behaviour.breaking = getBlockProperties(BlockType::TallGrass);
    else if (containsAny(name, {"glass", "pane"}))
        behaviour.breaking = getBlockProperties(BlockType::Glass);
    else if (containsAny(name, {"iron", "gold", "anvil"}))
        behaviour.breaking = getBlockProperties(BlockType::IronBlock);
    if (name.find("lava") != std::string_view::npos)
        behaviour.lightEmission = 15;
    if (containsAny(name, {"torch", "fire", "glowstone", "sea_lantern"}))
        behaviour.lightEmission = name.find("redstone") != std::string_view::npos ? 7 : 15;
    if (containsAny(name, {"bedrock", "barrier", "command_block", "structure_block"}))
        behaviour.dropRule = BlockDropRule::None;

    behaviour.replaceable =
        plant || portal || name == "air" || name == "snow_layer" ||
        name == "vine" || name == "water" || name == "flowing_water" ||
        name == "lava" || name == "flowing_lava";
    behaviour.gravityAffected =
        name == "sand" || name == "gravel" || name == "anvil" ||
        endsWith(name, "_concrete_powder");
    behaviour.randomTicks =
        plant || leaves || name == "grass" || name == "mycelium" ||
        name == "ice" || name == "snow_layer" || name == "fire" ||
        name == "cactus" || name == "reeds";
    behaviour.requiresSupport =
        plant || rail || component || name == "snow_layer" ||
        name == "cactus" || name == "reeds";
    behaviour.blockEntityType = blockEntityTypeForBlock(name);

    return behaviour;
}

void registerStructureCompatibilityBlocks(ContentCatalog& catalog)
{
    const auto addSynthetic=[&](std::string_view path,
        std::vector<std::vector<std::pair<std::string,std::string>>> states)
    {
        const core::ResourceLocation name=id(path);
        if(catalog.blocks().find(name)!=nullptr) return;
        BlockStateSchema schema; schema.states=std::move(states);
        BlockBehaviour behaviour=resourceBehaviour(path);
        behaviour.traits.opaque=false; behaviour.lightOpacity=0;
        catalog.registerBlock(name,{
            std::nullopt,displayNameFor(std::string(path)),{},std::move(schema),
            behaviour,RenderLayer::Cutout
        });
    };
    const auto horizontal=[]()
    {
        std::vector<std::vector<std::pair<std::string,std::string>>> out;
        for(const char* f:{"north","east","south","west"})
            out.push_back({{"facing",f}});
        return out;
    };
    std::vector<std::vector<std::pair<std::string,std::string>>> bedStates;
    for(const char* facing:{"north","east","south","west"})
        for(const char* part:{"foot","head"})
            bedStates.push_back({{"facing",facing},{"part",part}});
    addSynthetic("bed",std::move(bedStates));

    addSynthetic("wall_sign",horizontal());
    addSynthetic("wall_banner",horizontal());
    addSynthetic("ender_chest",horizontal());
    addSynthetic("trapped_chest",horizontal());
    std::vector<std::vector<std::pair<std::string,std::string>>> skull;
    for(const char* f:{"down","up","north","south","west","east"})
        skull.push_back({{"facing",f}});
    addSynthetic("skull",std::move(skull));
}

std::vector<std::pair<std::string,std::string>> itemVariantProps(std::string_view key)
{
    std::vector<std::pair<std::string,std::string>> r; if(key.empty()||key=="normal") return r; std::size_t b=0;
    while(b<key.size()){const std::size_t e=key.find(',',b);const auto part=key.substr(b,e==std::string_view::npos?key.size()-b:e-b);const std::size_t q=part.find('=');if(q!=std::string_view::npos)r.emplace_back(std::string(part.substr(0,q)),std::string(part.substr(q+1)));if(e==std::string_view::npos)break;b=e+1;} return r;
}
struct ResourceItemPlacement{core::ResourceLocation block{"minecraft:air"};std::vector<std::pair<std::string,std::string>> properties;};
bool noItemForm(std::string_view n){return equalsAny(n,{"air","flowing_water","water","flowing_lava","lava","double_stone_slab","double_wooden_slab","fire","portal","end_portal","lit_furnace","lit_redstone_ore","unlit_redstone_torch","powered_repeater","powered_comparator","standing_sign","wall_sign","piston_head","piston_extension","wheat","carrots","potatoes","beetroots","melon_stem","pumpkin_stem","cocoa"});}
void registerAllResourceItems(ContentCatalog& catalog,const resources::ResourcePack& pack)
{
    std::unordered_map<core::ResourceLocation,ResourceItemPlacement,core::ResourceLocationHash> owners;
    for(const auto& bn:pack.blockStateNames()){const auto bs=pack.loadBlockState(bn);for(const auto& [key,vars]:bs.variants){const auto props=itemVariantProps(key);for(const auto& v:vars)owners.try_emplace(v.model,ResourceItemPlacement{bn,props});}}
    for(const auto& name:pack.itemModelNames()){
        const core::ResourceLocation model(name.nameSpace(),std::string("item/")+name.path()); std::optional<core::ResourceLocation> placed; std::vector<std::pair<std::string,std::string>> props;
        if(catalog.blocks().find(name)) placed=name; else try{const auto raw=pack.loadModel(model);if(raw.parent){const auto it=owners.find(*raw.parent);if(it!=owners.end()){placed=it->second.block;props=it->second.properties;}}}catch(const std::exception&){}
        if(auto* existing=catalog.mutableItem(name)){existing->model=model;if(!existing->placedBlock&&placed){existing->placedBlock=placed;existing->placementProperties=std::move(props);}continue;}
        ItemProperties ip{};ip.name="Minecraft Item";ip.maximumStackSize=64;catalog.registerItem(name,{std::nullopt,displayNameFor(name.path()),placed,ip,model,std::move(props)});
    }
    for(const auto& bn:pack.blockStateNames()){if(!catalog.blocks().find(bn)||catalog.items().find(bn)||noItemForm(bn.path()))continue;ItemProperties ip{};ip.name="Minecraft Block";ip.maximumStackSize=64;catalog.registerItem(bn,{std::nullopt,displayNameFor(bn.path()),bn,ip,std::nullopt,{}});}
}

void registerResourceContent(
    ContentCatalog& catalog,
    const std::filesystem::path& assetRoot)
{
    if (assetRoot.empty())
        return;
    const resources::ResourcePack pack(assetRoot);
    for (const core::ResourceLocation& name : pack.lootTableNames())
    {
        if (catalog.lootTables().find(name) == nullptr)
            catalog.registerLootTable(name, {BlockDropRule::None});
    }
    for (const core::ResourceLocation& name : pack.blockStateNames())
    {
        if (catalog.blocks().find(name) != nullptr)
            continue;
        BlockStateSchema schema;
        schema.states = pack.blockStateCombinations(name);
        const BlockBehaviour behaviour = resourceBehaviour(name.path());
        const RenderLayer layer = behaviour.traits.translucent
            ? RenderLayer::Translucent
            : (behaviour.traits.leaf ? RenderLayer::CutoutMipped
                : (behaviour.traits.cutout ? RenderLayer::Cutout
                    : RenderLayer::Solid));
        catalog.registerBlock(
            name,
            {
                std::nullopt,
                displayNameFor(name.path()),
                {},
                std::move(schema),
                behaviour,
                layer
            }
        );
    }
    registerAllResourceItems(catalog, pack);
}
}

MinecraftContentModule::MinecraftContentModule(std::filesystem::path assetRoot)
    : assetRoot_(std::move(assetRoot))
{
}

core::ResourceLocation MinecraftContentModule::id() const
{
    return core::ResourceLocation("minecraft:builtins");
}

void MinecraftContentModule::registerContent(ContentCatalog& catalog)
{
    registerMinecraftContent(catalog, assetRoot_);
}

void registerMinecraftContent(
    ContentCatalog& catalog,
    const std::filesystem::path& assetRoot)
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
    addLoot("jungle_sapling", BlockDropRule::JungleSapling);
    addLoot("acacia_sapling", BlockDropRule::AcaciaSapling);
    addLoot("dark_oak_sapling", BlockDropRule::DarkOakSapling);
    addLoot("furnace", BlockDropRule::Furnace);
    addLoot("farmland", BlockDropRule::Farmland);
    addLoot("glass_like", BlockDropRule::GlassLike);
    addLoot("snowball", BlockDropRule::Snowball);
    addLoot("glowstone_dust", BlockDropRule::GlowstoneDust);
    addLoot("redstone_dust", BlockDropRule::RedstoneDust);
    addLoot("wheat_crop", BlockDropRule::WheatCrop);
    addLoot("carrot_crop", BlockDropRule::CarrotCrop);
    addLoot("potato_crop", BlockDropRule::PotatoCrop);
    addLoot("beetroot_crop", BlockDropRule::BeetrootCrop);
    addLoot("melon_slices", BlockDropRule::MelonSlices);
    addLoot("cocoa_beans", BlockDropRule::CocoaBeans);
    addLoot("books", BlockDropRule::Books);
    addLoot("cobweb_string", BlockDropRule::CobwebString);
    addLoot("dead_bush_sticks", BlockDropRule::DeadBushSticks);
    addLoot("mushroom_cap", BlockDropRule::MushroomCap);

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
    addBlock(catalog, BlockType::Fern, "fern", "Fern", cube("fern"));
    addBlock(catalog, BlockType::DeadBush, "deadbush", "Dead Bush", cube("deadbush"));
    addBlock(catalog, BlockType::Melon, "melon_block", "Melon", topSideBottom("melon_side", "melon_top", "melon_top"));
    addBlock(catalog, BlockType::Vine, "vine", "Vines", cube("vine"));
    addBlock(catalog, BlockType::Cocoa, "cocoa", "Cocoa", cube("cocoa_stage_2"));
    addBlock(catalog, BlockType::BrownMushroomBlock, "brown_mushroom_block", "Brown Mushroom Block", cube("mushroom_block_skin_brown"));
    addBlock(catalog, BlockType::RedMushroomBlock, "red_mushroom_block", "Red Mushroom Block", cube("mushroom_block_skin_red"));
    addBlock(catalog, BlockType::MushroomStem, "mushroom_stem", "Mushroom Stem", cube("mushroom_block_skin_stem"));
    addBlock(catalog, BlockType::StoneBricks, "stonebrick", "Stone Bricks", cube("stonebrick"));
    addBlock(catalog, BlockType::Bookshelf, "bookshelf", "Bookshelf", cube("bookshelf"));
    addBlock(catalog, BlockType::Cobweb, "web", "Cobweb", cube("web"));

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
    addItem(catalog, ItemType::String, "string", "String");
    addItem(catalog, ItemType::GlowstoneDust, "glowstone_dust", "Glowstone Dust");
    addItem(catalog, ItemType::Snowball, "snowball", "Snowball");
    addItem(catalog, ItemType::WheatItem, "wheat", "Wheat");
    addItem(catalog, ItemType::BeetrootItem, "beetroot", "Beetroot");
    addItem(catalog, ItemType::BeetrootSeeds, "beetroot_seeds", "Beetroot Seeds");
    addItem(catalog, ItemType::MelonSlice, "melon", "Melon");
    addItem(catalog, ItemType::CocoaBeans, "cocoa_beans", "Cocoa Beans");
    addItem(catalog, ItemType::NetherBrickItem, "netherbrick", "Nether Brick");
    addItem(catalog, ItemType::Book, "book", "Book");
    addItem(catalog, ItemType::JungleSapling, "jungle_sapling", "Jungle Sapling");
    addItem(catalog, ItemType::AcaciaSapling, "acacia_sapling", "Acacia Sapling");
    addItem(catalog, ItemType::DarkOakSapling, "dark_oak_sapling", "Dark Oak Sapling");
    addItem(catalog, ItemType::Arrow, "arrow", "Arrow");
    addItem(catalog, ItemType::RawBeef, "beef", "Raw Beef");
    addItem(catalog, ItemType::BlazeRod, "blaze_rod", "Blaze Rod");
    addItem(catalog, ItemType::Bone, "bone", "Bone");
    addItem(catalog, ItemType::RawChicken, "chicken", "Raw Chicken");
    addItem(catalog, ItemType::Emerald, "emerald", "Emerald");
    addItem(catalog, ItemType::EnderPearl, "ender_pearl", "Ender Pearl");
    addItem(catalog, ItemType::Feather, "feather", "Feather");
    addItem(catalog, ItemType::RawFish, "fish", "Raw Fish");
    addItem(catalog, ItemType::GhastTear, "ghast_tear", "Ghast Tear");
    addItem(catalog, ItemType::GlassBottle, "glass_bottle", "Glass Bottle");
    addItem(catalog, ItemType::GoldNugget, "gold_nugget", "Gold Nugget");
    addItem(catalog, ItemType::Gunpowder, "gunpowder", "Gunpowder");
    addItem(catalog, ItemType::Leather, "leather", "Leather");
    addItem(catalog, ItemType::MagmaCream, "magma_cream", "Magma Cream");
    addItem(catalog, ItemType::RawMutton, "mutton", "Raw Mutton");
    addItem(catalog, ItemType::NetherStar, "nether_star", "Nether Star");
    addItem(catalog, ItemType::RawPorkchop, "porkchop", "Raw Porkchop");
    addItem(catalog, ItemType::PrismarineCrystals, "prismarine_crystals", "Prismarine Crystals");
    addItem(catalog, ItemType::PrismarineShard, "prismarine_shard", "Prismarine Shard");
    addItem(catalog, ItemType::RawRabbit, "rabbit", "Raw Rabbit");
    addItem(catalog, ItemType::RabbitFoot, "rabbit_foot", "Rabbit's Foot");
    addItem(catalog, ItemType::RabbitHide, "rabbit_hide", "Rabbit Hide");
    addItem(catalog, ItemType::RottenFlesh, "rotten_flesh", "Rotten Flesh");
    addItem(catalog, ItemType::ShulkerShell, "shulker_shell", "Shulker Shell");
    addItem(catalog, ItemType::SlimeBall, "slime_ball", "Slimeball");
    addItem(catalog, ItemType::SpiderEye, "spider_eye", "Spider Eye");
    addItem(catalog, ItemType::Sugar, "sugar", "Sugar");
    addItem(catalog, ItemType::TippedArrow, "tipped_arrow", "Tipped Arrow");
    addItem(catalog, ItemType::TotemOfUndying, "totem_of_undying", "Totem of Undying");
    addItem(catalog, ItemType::Cookie, "cookie", "Cookie");
    addItem(catalog, ItemType::GoldenApple, "golden_apple", "Golden Apple");
    addItem(catalog, ItemType::GoldenCarrot, "golden_carrot", "Golden Carrot");
    addItem(catalog, ItemType::MelonSeeds, "melon_seeds", "Melon Seeds");
    addItem(catalog, ItemType::PumpkinSeeds, "pumpkin_seeds", "Pumpkin Seeds");
    addItem(catalog, ItemType::Saddle, "saddle", "Saddle");
    addItem(catalog, ItemType::Shears, "shears", "Shears");
    addItem(catalog, ItemType::CookedChicken, "cooked_chicken", "Cooked Chicken");
    addItem(catalog, ItemType::CookedFish, "cooked_fish", "Cooked Fish");
    addItem(catalog, ItemType::CookedMutton, "cooked_mutton", "Cooked Mutton");
    addItem(catalog, ItemType::CookedPorkchop, "cooked_porkchop", "Cooked Porkchop");
    addItem(catalog, ItemType::CookedRabbit, "cooked_rabbit", "Cooked Rabbit");
    addItem(catalog, ItemType::IronHorseArmor, "iron_horse_armor", "Iron Horse Armor");
    addItem(catalog, ItemType::GoldenHorseArmor, "golden_horse_armor", "Golden Horse Armor");
    addItem(catalog, ItemType::DiamondHorseArmor, "diamond_horse_armor", "Diamond Horse Armor");
    addItem(catalog, ItemType::Lead, "lead", "Lead");

    catalog.registerEntityType(id("item"), {"Dropped Item"});

    constexpr std::array<std::pair<std::string_view, std::string_view>, 25>
        vanillaBlockEntities{{
            {"furnace", "Furnace"},
            {"chest", "Chest"},
            {"ender_chest", "Ender Chest"},
            {"jukebox", "Jukebox"},
            {"dispenser", "Dispenser"},
            {"dropper", "Dropper"},
            {"sign", "Sign"},
            {"mob_spawner", "Mob Spawner"},
            {"noteblock", "Note Block"},
            {"piston", "Piston"},
            {"brewing_stand", "Brewing Stand"},
            {"enchanting_table", "Enchanting Table"},
            {"end_portal", "End Portal"},
            {"beacon", "Beacon"},
            {"skull", "Skull"},
            {"daylight_detector", "Daylight Detector"},
            {"hopper", "Hopper"},
            {"comparator", "Comparator"},
            {"flower_pot", "Flower Pot"},
            {"banner", "Banner"},
            {"structure_block", "Structure Block"},
            {"end_gateway", "End Gateway"},
            {"command_block", "Command Block"},
            {"shulker_box", "Shulker Box"},
            {"bed", "Bed"}
        }};

    for (const auto& [name, display] : vanillaBlockEntities)
        catalog.registerBlockEntityType(
            id(name), {std::string(display), 1});

    registerResourceContent(catalog, assetRoot);
    registerStructureCompatibilityBlocks(catalog);
}
}
