#include "worldgen/Vanilla112State.h"

#include "content/ContentCatalog.h"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace mc112
{
namespace
{
const mc::content::ContentCatalog& catalog()
{
    const auto* result = mc::content::ContentCatalog::active();
    if (result == nullptr)
        throw std::logic_error("Minecraft content catalog is not active");
    return *result;
}

std::string resourcePath(std::string_view name)
{
    const std::size_t colon = name.find(':');
    return std::string(colon == std::string_view::npos ? name : name.substr(colon + 1));
}

bool pathEquals(mc::content::BlockState value, std::string_view wanted) noexcept
{
    const auto* active = mc::content::ContentCatalog::active();
    if (active == nullptr)
        return false;
    const auto* name = active->blockName(value);
    return name != nullptr && name->nameSpace() == "minecraft" && name->path() == wanted;
}

std::string property(std::span<const Property> props, std::string_view name, std::string_view fallback = {})
{
    for (const auto& [key, value] : props)
        if (key == name)
            return value;
    return std::string(fallback);
}

std::vector<Property> without(
    std::span<const Property> props,
    std::initializer_list<std::string_view> removed)
{
    std::vector<Property> result;
    result.reserve(props.size());
    for (const auto& item : props)
    {
        bool skip = false;
        for (std::string_view name : removed)
            skip = skip || item.first == name;
        if (!skip)
            result.push_back(item);
    }
    return result;
}

std::string colorName(std::string value)
{
    if (value == "silver") return "light_gray";
    return value;
}

std::optional<bool> boolProperty(std::span<const Property> props, std::string_view name)
{
    for (const auto& [key, value] : props)
    {
        if (key != name)
            continue;
        if (value == "true") return true;
        if (value == "false") return false;
        return std::nullopt;
    }
    return std::nullopt;
}

std::optional<int> integerProperty(std::span<const Property> props, std::string_view name)
{
    for (const auto& [key, value] : props)
    {
        if (key != name)
            continue;
        try
        {
            std::size_t used = 0;
            const int result = std::stoi(value, &used);
            return used == value.size() ? std::optional<int>(result) : std::nullopt;
        }
        catch (...)
        {
            return std::nullopt;
        }
    }
    return std::nullopt;
}

std::optional<mc::content::BlockState> legacyMetadataState(
    std::string_view name,
    std::span<const Property> props)
{
    const std::string base = resourcePath(name);
    const auto value = [&](std::string_view key, std::string_view fallback = {})
    {
        return property(props, key, fallback);
    };

    if (base == "lever")
    {
        static const std::unordered_map<std::string, int> orientation{
            {"down_x",0},{"east",1},{"west",2},{"south",3},
            {"north",4},{"up_z",5},{"up_x",6},{"down_z",7}
        };
        const std::string facing = value("facing", "north");
        const auto it = orientation.find(facing);
        if (it == orientation.end()) return std::nullopt;
        const bool powered = boolProperty(props, "powered").value_or(false);
        return mc::content::BlockState(
            BlockType::Lever,
            static_cast<std::uint16_t>(it->second | (powered ? 8 : 0)));
    }

    if (base == "piston" || base == "sticky_piston")
    {
        static const std::unordered_map<std::string, int> facingIndex{
            {"down",0},{"up",1},{"north",2},{"south",3},{"west",4},{"east",5}
        };
        const std::string facing = value("facing", "north");
        const auto it = facingIndex.find(facing);
        if (it == facingIndex.end()) return std::nullopt;
        const bool extended = boolProperty(props, "extended").value_or(false);
        return mc::content::BlockState(
            base == "piston" ? BlockType::Piston : BlockType::StickyPiston,
            static_cast<std::uint16_t>(it->second | (extended ? 8 : 0)));
    }

    if (base == "unpowered_repeater")
    {
        static const std::unordered_map<std::string, int> horizontalIndex{
            {"south",0},{"west",1},{"north",2},{"east",3}
        };
        const std::string facing = value("facing", "north");
        const auto it = horizontalIndex.find(facing);
        if (it == horizontalIndex.end()) return std::nullopt;
        const int delay = integerProperty(props, "delay").value_or(1);
        if (delay < 1 || delay > 4) return std::nullopt;
        if (boolProperty(props, "locked").value_or(false))
            return std::nullopt; // LOCKED is an actual-state property, not metadata.
        return mc::content::BlockState(
            BlockType::Repeater,
            static_cast<std::uint16_t>(it->second | ((delay - 1) << 2)));
    }

    if (base == "vine")
    {
        // BlockVine metadata stores S/W/N/E in bits 0/1/2/3. UP is
        // computed by getActualState and is intentionally not persisted.
        if (boolProperty(props, "up").value_or(false))
            return std::nullopt;
        int metadata = 0;
        if (boolProperty(props, "south").value_or(false)) metadata |= 1;
        if (boolProperty(props, "west").value_or(false)) metadata |= 2;
        if (boolProperty(props, "north").value_or(false)) metadata |= 4;
        if (boolProperty(props, "east").value_or(false)) metadata |= 8;
        return mc::content::BlockState(
            BlockType::Vine, static_cast<std::uint16_t>(metadata));
    }

    if (base == "redstone_wire")
    {
        // Cardinal connection properties are getActualState output; only power
        // is stored in metadata. Reject impossible attempts to persist them.
        for (std::string_view key : {"north","east","south","west"})
        {
            const std::string connection = value(key);
            if (!connection.empty() && connection != "none")
                return std::nullopt;
        }
        const int power = integerProperty(props, "power").value_or(0);
        if (power < 0 || power > 15) return std::nullopt;
        return mc::content::BlockState(
            BlockType::RedstoneWire, static_cast<std::uint16_t>(power));
    }

    return std::nullopt;
}

std::string splitVariantName(
    const std::string& base,
    std::span<const Property> props,
    std::vector<Property>& remaining)
{
    remaining.assign(props.begin(), props.end());
    auto drop = [&remaining](std::initializer_list<std::string_view> keys)
    {
        remaining.erase(
            std::remove_if(remaining.begin(), remaining.end(), [&](const Property& p)
            {
                return std::any_of(keys.begin(), keys.end(), [&](std::string_view k)
                { return p.first == k; });
            }),
            remaining.end()
        );
    };

    const std::string variant = property(props, "variant");
    const std::string type = property(props, "type");
    const std::string color = colorName(property(props, "color"));

    if (base == "stone" && !variant.empty())
    {
        drop({"variant"});
        if (variant == "stone") return "stone";
        if (variant == "granite") return "granite";
        if (variant == "smooth_granite") return "polished_granite";
        if (variant == "diorite") return "diorite";
        if (variant == "smooth_diorite") return "polished_diorite";
        if (variant == "andesite") return "andesite";
        if (variant == "smooth_andesite") return "polished_andesite";
    }
    if (base == "dirt" && !variant.empty())
    {
        drop({"variant", "snowy"});
        if (variant == "dirt") return "dirt";
        if (variant == "coarse_dirt") return "coarse_dirt";
        if (variant == "podzol") return "podzol";
    }
    if (base == "planks" && !variant.empty())
    {
        drop({"variant"});
        return variant + "_planks";
    }
    if (base == "sapling" && !type.empty())
    {
        drop({"type", "stage"});
        return type + "_sapling";
    }
    if ((base == "log" || base == "log2") && !variant.empty())
    {
        drop({"variant"});
        return variant + "_log";
    }
    if ((base == "leaves" || base == "leaves2") && !variant.empty())
    {
        drop({"variant", "check_decay", "decayable"});
        return variant + "_leaves";
    }
    if (base == "sand" && !variant.empty())
    {
        drop({"variant"});
        return variant == "red_sand" ? "red_sand" : "sand";
    }
    if (base == "sandstone" && !type.empty())
    {
        drop({"type"});
        if (type == "chiseled") return "chiseled_sandstone";
        if (type == "smooth") return "smooth_sandstone";
        return "sandstone";
    }
    if (base == "red_sandstone" && !type.empty())
    {
        drop({"type"});
        if (type == "chiseled") return "chiseled_red_sandstone";
        if (type == "smooth") return "smooth_red_sandstone";
        return "red_sandstone";
    }
    if (base == "wool" && !color.empty())
    {
        drop({"color"});
        return color + "_wool";
    }
    if (base == "carpet" && !color.empty())
    {
        drop({"color"});
        return color + "_carpet";
    }
    if (base == "stained_hardened_clay" && !color.empty())
    {
        drop({"color"});
        return color + "_stained_hardened_clay";
    }
    if (base == "stained_glass" && !color.empty())
    {
        drop({"color"});
        return color + "_stained_glass";
    }
    if (base == "stained_glass_pane" && !color.empty())
    {
        drop({"color"});
        return color + "_stained_glass_pane";
    }
    if (base == "concrete" && !color.empty())
    {
        drop({"color"});
        return color + "_concrete";
    }
    if (base == "concrete_powder" && !color.empty())
    {
        drop({"color"});
        return color + "_concrete_powder";
    }
    if (base == "stonebrick" && !variant.empty())
    {
        drop({"variant"});
        if (variant == "mossy_stonebrick") return "mossy_stonebrick";
        if (variant == "cracked_stonebrick") return "cracked_stonebrick";
        if (variant == "chiseled_stonebrick") return "chiseled_stonebrick";
        return "stonebrick";
    }
    if (base == "prismarine" && !variant.empty())
    {
        drop({"variant"});
        if (variant == "prismarine_bricks") return "prismarine_bricks";
        if (variant == "dark_prismarine") return "dark_prismarine";
        return "prismarine";
    }
    if (base == "red_flower" && !type.empty())
    {
        drop({"type"});
        static const std::unordered_map<std::string, std::string> names{
            {"poppy","poppy"},{"blue_orchid","blue_orchid"},{"allium","allium"},
            {"houstonia","azure_bluet"},{"red_tulip","red_tulip"},
            {"orange_tulip","orange_tulip"},{"white_tulip","white_tulip"},
            {"pink_tulip","pink_tulip"},{"oxeye_daisy","oxeye_daisy"}
        };
        if (const auto it = names.find(type); it != names.end()) return it->second;
    }
    if (base == "tallgrass" && !type.empty())
    {
        drop({"type"});
        if (type == "fern") return "fern";
        if (type == "dead_bush") return "deadbush";
        return "tall_grass";
    }
    if (base == "double_plant" && !variant.empty())
    {
        drop({"variant"});
        static const std::unordered_map<std::string, std::string> names{
            {"sunflower","sunflower"},{"syringa","lilac"},
            {"double_grass","double_tallgrass"},{"double_fern","large_fern"},
            {"double_rose","rose_bush"},{"paeonia","peony"}
        };
        if (const auto it = names.find(variant); it != names.end()) return it->second;
    }
    if ((base == "stone_slab" || base == "double_stone_slab") && !variant.empty())
    {
        drop({"variant", "seamless"});
        static const std::unordered_map<std::string, std::string> names{
            {"stone","stone"},{"sandstone","sandstone"},{"wood_old","petrified_oak"},
            {"cobblestone","cobblestone"},{"brick","brick"},{"stone_brick","stone_brick"},
            {"nether_brick","nether_brick"},{"quartz","quartz"}
        };
        const auto it = names.find(variant);
        if (it != names.end())
            return it->second + (base == "double_stone_slab" ? "_double_slab" : "_slab");
    }
    if ((base == "wooden_slab" || base == "double_wooden_slab") && !variant.empty())
    {
        drop({"variant"});
        return variant + (base == "double_wooden_slab" ? "_double_slab" : "_slab");
    }
    if ((base == "stone_slab2" || base == "double_stone_slab2") && !variant.empty())
    {
        drop({"variant", "seamless"});
        return std::string("red_sandstone") + (base == "double_stone_slab2" ? "_double_slab" : "_slab");
    }
    if (base == "monster_egg" && !variant.empty())
    {
        drop({"variant"});
        static const std::unordered_map<std::string,std::string> names{
            {"stone","stone_monster_egg"},{"cobblestone","cobblestone_monster_egg"},
            {"stone_brick","stone_brick_monster_egg"},{"mossy_brick","mossy_stone_brick_monster_egg"},
            {"cracked_brick","cracked_stone_brick_monster_egg"},{"chiseled_brick","chiseled_stone_brick_monster_egg"}
        };
        if (const auto it=names.find(variant);it!=names.end()) return it->second;
    }
    if (base == "quartz_block" && !variant.empty())
    {
        drop({"variant"});
        if (variant == "chiseled") return "chiseled_quartz_block";
        if (variant.rfind("lines_",0)==0)
        {
            remaining.emplace_back("axis", variant == "lines_x" ? "x" : variant == "lines_z" ? "z" : "y");
            return "quartz_pillar";
        }
        return "quartz_block";
    }

    // The resource pack retains these historic registry paths verbatim.
    return base;
}
}

std::optional<mc::content::BlockState> tryState(
    std::string_view name,
    std::span<const Property> properties)
{
    if (auto legacy = legacyMetadataState(name, properties); legacy)
        return legacy;

    const mc::core::ResourceLocation id(
        name.find(':') == std::string_view::npos
            ? std::string("minecraft:") + std::string(name)
            : std::string(name)
    );
    if (properties.empty())
    {
        const auto direct = catalog().state(id, 0);
        if (direct) return direct;
        const auto defaultState = catalog().defaultState(id);
        return catalog().block(defaultState) == nullptr ? std::nullopt
            : std::optional<mc::content::BlockState>(defaultState);
    }
    return catalog().state(id, properties);
}

std::optional<mc::content::BlockState> tryState(
    std::string_view name,
    std::initializer_list<Property> properties)
{
    return tryState(name, std::span<const Property>(properties.begin(), properties.size()));
}

mc::content::BlockState state(std::string_view name, std::span<const Property> properties)
{
    const auto result = tryState(name, properties);
    if (!result)
        throw std::runtime_error("Missing exact Minecraft 1.12 block state: " + std::string(name));
    return *result;
}

mc::content::BlockState state(
    std::string_view name,
    std::initializer_list<Property> properties)
{
    return state(name, std::span<const Property>(properties.begin(), properties.size()));
}

std::optional<mc::content::BlockState> tryVanilla112State(
    std::string_view registryName,
    std::span<const Property> properties)
{
    const std::string base = resourcePath(registryName);
    std::vector<Property> remaining;
    const std::string mapped = splitVariantName(base, properties, remaining);

    if (auto exact = tryState(mapped, std::span<const Property>(remaining)); exact)
        return exact;

    // Never discard a 1.12 property just to obtain the right block identity.
    // Rendering-only resource JSON omits legitimate stored properties for
    // several vanilla blocks (redstone power is the classic example). Those
    // states must be supplied by the legacy/state bridge instead of silently
    // collapsing to a resource default.
    if (mapped != base)
        return std::nullopt;

    return tryState(base, properties);
}

mc::content::BlockState vanilla112State(
    std::string_view registryName,
    std::span<const Property> properties)
{
    const auto result = tryVanilla112State(registryName, properties);
    if (!result)
        throw std::runtime_error(
            "Missing Minecraft 1.12.2 palette state: " + std::string(registryName)
        );
    return *result;
}

bool named(mc::content::BlockState value, std::string_view name) noexcept
{
    return pathEquals(value, resourcePath(name));
}

std::string_view path(mc::content::BlockState value) noexcept
{
    const auto* active = mc::content::ContentCatalog::active();
    if (active == nullptr) return {};
    const auto* name = active->blockName(value);
    return name == nullptr ? std::string_view{} : std::string_view(name->path());
}

bool isAir(mc::content::BlockState value) noexcept { return value.isAir() || named(value,"air"); }
bool isWater(mc::content::BlockState value) noexcept { return named(value,"water") || named(value,"flowing_water") || value.block()==BlockType::Water; }
bool isLava(mc::content::BlockState value) noexcept { return named(value,"lava") || named(value,"flowing_lava") || value.block()==BlockType::Lava; }
bool isLiquid(mc::content::BlockState value) noexcept
{
    if(isWater(value)||isLava(value))return true;
    const auto* a=mc::content::ContentCatalog::active();const auto* d=a? a->block(value):nullptr;
    return d&&d->behaviour.traits.liquid;
}
bool isLeaf(mc::content::BlockState value) noexcept
{
    if(::isLeaf(value.block()))return true;const auto* a=mc::content::ContentCatalog::active();const auto* d=a?a->block(value):nullptr;return d&&d->behaviour.traits.leaf;
}
bool isLog(mc::content::BlockState value) noexcept
{
    if(::isLog(value.block()))return true;const auto p=path(value);return p=="log"||p=="log2"||p.ends_with("_log")||p.ends_with("_wood");
}
bool isSolid(mc::content::BlockState value) noexcept
{
    const auto* a=mc::content::ContentCatalog::active();const auto* d=a?a->block(value):nullptr;return d ? d->behaviour.traits.solid : ::isSolid(value.block());
}
bool isOpaque(mc::content::BlockState value) noexcept
{
    const auto* a=mc::content::ContentCatalog::active();const auto* d=a?a->block(value):nullptr;return d ? d->behaviour.traits.opaque : ::isOpaque(value.block());
}
bool isPlant(mc::content::BlockState value) noexcept
{
    const auto* a=mc::content::ContentCatalog::active();const auto* d=a?a->block(value):nullptr;return d&&d->behaviour.traits.plant;
}
bool isReplaceableByStructure(mc::content::BlockState value) noexcept
{
    return isAir(value)||isLiquid(value)||isPlant(value)||isLeaf(value)||named(value,"snow_layer")||named(value,"vine")||named(value,"tallgrass")||named(value,"tall_grass")||named(value,"deadbush");
}
bool isNaturalStone(mc::content::BlockState value) noexcept
{
    const auto p=path(value);return value.block()==BlockType::Stone||p=="stone"||p=="granite"||p=="diorite"||p=="andesite";
}
bool isDirtLike(mc::content::BlockState value) noexcept
{
    const auto p=path(value);return value.block()==BlockType::Dirt||value.block()==BlockType::Grass||value.block()==BlockType::Podzol||p=="dirt"||p=="coarse_dirt"||p=="podzol"||p=="grass"||p=="grass_block"||p=="mycelium"||p=="farmland"||p=="grass_path";
}
}
