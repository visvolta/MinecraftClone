#include "ItemAtlas.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string_view>
#include <vector>

#include <stb_image.h>

namespace
{
std::string_view itemTexture(ItemType item)
{
    switch (item)
    {
        case ItemType::Stick: return "items/stick";
        case ItemType::Coal: return "items/coal";
        case ItemType::Diamond: return "items/diamond";
        case ItemType::IronIngot: return "items/iron_ingot";
        case ItemType::GoldIngot: return "items/gold_ingot";
        case ItemType::Flint: return "items/flint";
        case ItemType::RedstoneDust: return "items/redstone_dust";
        case ItemType::ClayBall: return "items/clay_ball";
        case ItemType::LapisLazuli: return "items/dye_powder_blue";
        case ItemType::Seeds: return "items/seeds_wheat";
        case ItemType::OakSapling: return "blocks/sapling_oak";
        case ItemType::SpruceSapling: return "blocks/sapling_spruce";
        case ItemType::BirchSapling: return "blocks/sapling_birch";
        case ItemType::Charcoal: return "items/charcoal";
        case ItemType::Brick: return "items/brick";
        case ItemType::WoodenShovel: return "items/wood_shovel";
        case ItemType::WoodenPickaxe: return "items/wood_pickaxe";
        case ItemType::WoodenAxe: return "items/wood_axe";
        case ItemType::StoneShovel: return "items/stone_shovel";
        case ItemType::StonePickaxe: return "items/stone_pickaxe";
        case ItemType::StoneAxe: return "items/stone_axe";
        case ItemType::IronShovel: return "items/iron_shovel";
        case ItemType::IronPickaxe: return "items/iron_pickaxe";
        case ItemType::IronAxe: return "items/iron_axe";
        case ItemType::DiamondShovel: return "items/diamond_shovel";
        case ItemType::DiamondPickaxe: return "items/diamond_pickaxe";
        case ItemType::DiamondAxe: return "items/diamond_axe";
        case ItemType::GoldenShovel: return "items/gold_shovel";
        case ItemType::GoldenPickaxe: return "items/gold_pickaxe";
        case ItemType::GoldenAxe: return "items/gold_axe";
        case ItemType::Apple: return "items/apple";
        case ItemType::Bread: return "items/bread";
        case ItemType::Carrot: return "items/carrot";
        case ItemType::Potato: return "items/potato";
        case ItemType::BakedPotato: return "items/potato_baked";
        case ItemType::CookedBeef: return "items/beef_cooked";
        case ItemType::Shield: return "items/empty_armor_slot_shield";
        case ItemType::IronHelmet: return "items/iron_helmet";
        case ItemType::IronChestplate: return "items/iron_chestplate";
        case ItemType::IronLeggings: return "items/iron_leggings";
        case ItemType::IronBoots: return "items/iron_boots";
        case ItemType::DiamondHelmet: return "items/diamond_helmet";
        case ItemType::DiamondChestplate: return "items/diamond_chestplate";
        case ItemType::DiamondLeggings: return "items/diamond_leggings";
        case ItemType::DiamondBoots: return "items/diamond_boots";
        case ItemType::String: return "items/string";
        case ItemType::GlowstoneDust: return "items/glowstone_dust";
        case ItemType::Snowball: return "items/snowball";
        case ItemType::WheatItem: return "items/wheat";
        case ItemType::BeetrootItem: return "items/beetroot";
        case ItemType::BeetrootSeeds: return "items/beetroot_seeds";
        case ItemType::MelonSlice: return "items/melon";
        case ItemType::CocoaBeans: return "items/dye_powder_brown";
        case ItemType::NetherBrickItem: return "items/netherbrick";
        case ItemType::Book: return "items/book_normal";
        case ItemType::JungleSapling: return "blocks/sapling_jungle";
        case ItemType::AcaciaSapling: return "blocks/sapling_acacia";
        case ItemType::DarkOakSapling: return "blocks/sapling_roofed_oak";
        case ItemType::Arrow: return "items/arrow";
        case ItemType::RawBeef: return "items/beef_raw";
        case ItemType::BlazeRod: return "items/blaze_rod";
        case ItemType::Bone: return "items/bone";
        case ItemType::RawChicken: return "items/chicken_raw";
        case ItemType::Dye: return "items/dye_powder_black";
        case ItemType::Emerald: return "items/emerald";
        case ItemType::EnderPearl: return "items/ender_pearl";
        case ItemType::Feather: return "items/feather";
        case ItemType::RawFish: return "items/fish_cod_raw";
        case ItemType::GhastTear: return "items/ghast_tear";
        case ItemType::GlassBottle: return "items/potion_bottle_empty";
        case ItemType::GoldNugget: return "items/gold_nugget";
        case ItemType::Gunpowder: return "items/gunpowder";
        case ItemType::Leather: return "items/leather";
        case ItemType::MagmaCream: return "items/magma_cream";
        case ItemType::RawMutton: return "items/mutton_raw";
        case ItemType::NetherStar: return "items/nether_star";
        case ItemType::RawPorkchop: return "items/porkchop_raw";
        case ItemType::PrismarineCrystals: return "items/prismarine_crystals";
        case ItemType::PrismarineShard: return "items/prismarine_shard";
        case ItemType::RawRabbit: return "items/rabbit_raw";
        case ItemType::RabbitFoot: return "items/rabbit_foot";
        case ItemType::RabbitHide: return "items/rabbit_hide";
        case ItemType::RottenFlesh: return "items/rotten_flesh";
        case ItemType::ShulkerShell: return "items/shulker_shell";
        case ItemType::SlimeBall: return "items/slimeball";
        case ItemType::SpiderEye: return "items/spider_eye";
        case ItemType::Sugar: return "items/sugar";
        case ItemType::TippedArrow: return "items/tipped_arrow_base";
        case ItemType::TotemOfUndying: return "items/totem";
        case ItemType::Empty: break;
    }
    return {};
}

void setPixel(
    std::vector<std::uint8_t>& pixels,
    int width,
    int x,
    int y,
    const std::uint8_t* colour)
{
    std::copy_n(
        colour,
        4,
        pixels.data() + static_cast<std::size_t>((y * width + x) * 4)
    );
}
}

ItemAtlas::ItemAtlas(const std::filesystem::path& textureRoot)
{
    std::vector<ItemType> items;
    const auto first = static_cast<std::uint16_t>(ItemType::Stick);
    const auto last = static_cast<std::uint16_t>(ItemType::TotemOfUndying);
    for (std::uint16_t value = first; value <= last; ++value)
    {
        const ItemType item = static_cast<ItemType>(value);
        if (!itemTexture(item).empty())
            items.push_back(item);
    }

    const int columns = static_cast<int>(std::ceil(
        std::sqrt(static_cast<double>(items.size()))
    ));
    const int rows = static_cast<int>(
        (items.size() + static_cast<std::size_t>(columns) - 1U) /
        static_cast<std::size_t>(columns)
    );
    const int width = columns * CELL_SIZE_PIXELS;
    const int height = rows * CELL_SIZE_PIXELS;
    std::vector<std::uint8_t> topDown(
        static_cast<std::size_t>(width * height * 4), 0U
    );

    for (std::size_t index = 0; index < items.size(); ++index)
    {
        const ItemType item = items[index];
        const std::filesystem::path path = textureRoot /
            (std::string(itemTexture(item)) + ".png");
        int sourceWidth = 0;
        int sourceHeight = 0;
        int channels = 0;
        stbi_set_flip_vertically_on_load(false);
        unsigned char* source = stbi_load(
            path.string().c_str(), &sourceWidth, &sourceHeight, &channels, 4
        );
        if (source == nullptr || sourceWidth < 16 || sourceHeight < 16)
        {
            if (source != nullptr)
                stbi_image_free(source);
            throw std::runtime_error("Missing 1.12 item texture: " + path.string());
        }

        const int cellX = static_cast<int>(index % static_cast<std::size_t>(columns)) * CELL_SIZE_PIXELS;
        const int cellY = static_cast<int>(index / static_cast<std::size_t>(columns)) * CELL_SIZE_PIXELS;
        auto& itemAlpha = alpha_[item];
        for (int y = 0; y < CELL_SIZE_PIXELS; ++y)
        {
            const int sourceY = std::clamp(y - GUTTER_PIXELS, 0, 15);
            for (int x = 0; x < CELL_SIZE_PIXELS; ++x)
            {
                const int sourceX = std::clamp(x - GUTTER_PIXELS, 0, 15);
                const auto* colour = source +
                    static_cast<std::size_t>((sourceY * sourceWidth + sourceX) * 4);
                setPixel(topDown, width, cellX + x, cellY + y, colour);
            }
        }
        for (int y = 0; y < 16; ++y)
        {
            for (int x = 0; x < 16; ++x)
            {
                itemAlpha[static_cast<std::size_t>(y * 16 + x)] = source[
                    static_cast<std::size_t>((y * sourceWidth + x) * 4 + 3)
                ];
            }
        }
        stbi_image_free(source);

        constexpr float inset = 0.01f;
        const int imageX = cellX + GUTTER_PIXELS;
        const int imageY = cellY + GUTTER_PIXELS;
        const int glBottom = height - imageY - 16;
        entries_.emplace(item, AtlasUV{
            (static_cast<float>(imageX) + inset) / static_cast<float>(width),
            (static_cast<float>(glBottom) + inset) / static_cast<float>(height),
            (static_cast<float>(imageX + 16) - inset) / static_cast<float>(width),
            (static_cast<float>(glBottom + 16) - inset) / static_cast<float>(height)
        });
    }

    std::vector<std::uint8_t> bottomUp(topDown.size());
    const std::size_t rowBytes = static_cast<std::size_t>(width * 4);
    for (int y = 0; y < height; ++y)
    {
        std::copy_n(
            topDown.data() + static_cast<std::size_t>(y) * rowBytes,
            rowBytes,
            bottomUp.data() + static_cast<std::size_t>(height - y - 1) * rowBytes
        );
    }
    texture_ = std::make_unique<Texture2D>(width, height, bottomUp);
}

const Texture2D& ItemAtlas::texture() const noexcept { return *texture_; }

AtlasUV ItemAtlas::getItemUV(ItemType item) const
{
    const auto found = entries_.find(item);
    if (found == entries_.end())
        throw std::invalid_argument("Item has no runtime item texture");
    return found->second;
}

bool ItemAtlas::isOpaque(ItemType item, int pixelX, int pixelY) const noexcept
{
    if (pixelX < 0 || pixelX >= 16 || pixelY < 0 || pixelY >= 16)
        return false;
    const auto found = alpha_.find(item);
    return found != alpha_.end() && found->second[
        static_cast<std::size_t>(pixelY * 16 + pixelX)
    ] >= 16U;
}
