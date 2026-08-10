#include "BlockDrops.h"

#include "BlockProperties.h"
#include "content/ContentCatalog.h"

namespace
{
int randomInclusive(std::mt19937& random, int minimum, int maximum)
{
    return std::uniform_int_distribution<int>(minimum, maximum)(random);
}

std::vector<ItemStack> individualDrops(ItemType item, int count)
{
    std::vector<ItemStack> drops;
    drops.reserve(static_cast<std::size_t>(count));
    for (int index = 0; index < count; ++index)
        drops.emplace_back(item, 1);
    return drops;
}

mc::content::BlockDropRule dropRuleFor(BlockType block) noexcept
{
    if (const mc::content::ContentCatalog* catalog =
            mc::content::ContentCatalog::active())
    {
        if (const mc::content::BlockDefinition* definition =
                catalog->block(block))
        {
            if (const mc::content::LootTableDefinition* loot =
                    catalog->lootTable(definition->behaviour.lootTable))
            {
                return loot->rule;
            }
        }
    }
    if (block == BlockType::Air || block == BlockType::Water ||
        block == BlockType::Lava || block == BlockType::Bedrock ||
        block == BlockType::Spawner)
        return mc::content::BlockDropRule::None;
    return mc::content::BlockDropRule::Self;
}
}

std::vector<ItemStack> getBlockDrops(
    BlockType block,
    std::uint8_t,
    const ToolProperties& tool,
    std::mt19937& random)
{
    if (!canHarvestBlock(block, tool))
        return {};

    switch (dropRuleFor(block))
    {
        case mc::content::BlockDropRule::None:
            return {};
        case mc::content::BlockDropRule::Cobblestone:
            return {ItemStack(BlockType::Cobblestone)};
        case mc::content::BlockDropRule::Dirt:
            return {ItemStack(BlockType::Dirt)};
        case mc::content::BlockDropRule::FlintOrGravel:
            return randomInclusive(random, 0, 9) == 0
                ? std::vector<ItemStack>{ItemStack(ItemType::Flint, 1)}
                : std::vector<ItemStack>{ItemStack(BlockType::Gravel)};
        case mc::content::BlockDropRule::ClayBalls:
            return individualDrops(ItemType::ClayBall, 4);
        case mc::content::BlockDropRule::Coal:
            return {ItemStack(ItemType::Coal, 1)};
        case mc::content::BlockDropRule::Diamond:
            return {ItemStack(ItemType::Diamond, 1)};
        case mc::content::BlockDropRule::Redstone:
            return individualDrops(
                ItemType::RedstoneDust,
                randomInclusive(random, 4, 5)
            );
        case mc::content::BlockDropRule::Lapis:
            return individualDrops(
                ItemType::LapisLazuli,
                randomInclusive(random, 4, 8)
            );
        case mc::content::BlockDropRule::Seeds:
            return randomInclusive(random, 0, 7) == 0
                ? std::vector<ItemStack>{ItemStack(ItemType::Seeds, 1)}
                : std::vector<ItemStack>{};
        case mc::content::BlockDropRule::OakSapling:
        {
            std::vector<ItemStack> drops;
            if (randomInclusive(random, 0, 19) == 0)
                drops.emplace_back(ItemType::OakSapling, 1);
            if (randomInclusive(random, 0, 199) == 0)
                drops.emplace_back(ItemType::Apple, 1);
            return drops;
        }
        case mc::content::BlockDropRule::SpruceSapling:
            return randomInclusive(random, 0, 19) == 0
                ? std::vector<ItemStack>{ItemStack(ItemType::SpruceSapling, 1)}
                : std::vector<ItemStack>{};
        case mc::content::BlockDropRule::BirchSapling:
            return randomInclusive(random, 0, 19) == 0
                ? std::vector<ItemStack>{ItemStack(ItemType::BirchSapling, 1)}
                : std::vector<ItemStack>{};
        case mc::content::BlockDropRule::Furnace:
            return {ItemStack(BlockType::Furnace)};
        case mc::content::BlockDropRule::Self:
            return {ItemStack(block)};
    }
    return {};
}
