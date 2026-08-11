#include "BlockEntity.h"
#include "BlockDrops.h"
#include "CraftingRecipe.h"
#include "Furnace.h"
#include "content/ContentCatalog.h"
#include "game/GameBootstrap.h"

#include <cassert>
#include <filesystem>
#include <random>

int main()
{
    mc::game::GameBootstrap bootstrap;
    bootstrap.loadContentModules();
    bootstrap.freezeRegistries();
    const mc::content::ContentCatalog& content = bootstrap.content();

    assert(content.entityTypes().size() == 50);
    assert(content.blockEntityTypes().size() == 3);
    assert(content.lootTables().size() == 33);
    assert(getItemProperties(ItemType::DiamondPickaxe).maximumDamage == 1561);
    assert(getItemProperties(ItemType::RawBeef).foodPoints == 3);
    assert(content.legacyItem(
        mc::core::ResourceLocation("minecraft:beef")
    ) == ItemType::RawBeef);

    std::mt19937 dropRandom(1122U);
    const ToolProperties hand{};
    assert(getBlockDrops(BlockType::Glass, 0, hand, dropRandom).empty());
    const auto farmlandDrops = getBlockDrops(
        BlockType::Farmland, 0, hand, dropRandom
    );
    assert(farmlandDrops.size() == 1U &&
           farmlandDrops.front().item == itemFromBlock(BlockType::Dirt));
    const auto wheatDrops = getBlockDrops(
        BlockType::Wheat, 7, hand, dropRandom
    );
    assert(!wheatDrops.empty() &&
           wheatDrops.front().item == ItemType::WheatItem);
    assert(getBlockDrops(
        BlockType::Bookshelf, 0, hand, dropRandom
    ).size() == 3U);

    const std::filesystem::path assets("assets");
    RecipeRegistry recipes(assets, content);
    assert(recipes.size() == 32);

    CraftingGrid grid{};
    grid[0] = {BlockType::OakPlanks};
    grid[1] = {BlockType::SprucePlanks};
    grid[3] = {BlockType::BirchPlanks};
    grid[4] = {BlockType::OakPlanks};
    const CraftingRecipe* recipe = recipes.findMatch(grid);
    assert(recipe != nullptr);
    assert(RecipeRegistry::getResult(*recipe).item ==
           itemFromBlock(BlockType::CraftingTable));

    FurnaceRecipeRegistry furnaceRecipes(assets, content);
    assert(furnaceRecipes.smeltingRecipeCount() == 8);
    assert(furnaceRecipes.fuelCount() == 14);
    assert(furnaceRecipes.smeltingResult(
        itemFromBlock(BlockType::IronOre)).item == ItemType::IronIngot);
    assert(furnaceRecipes.fuelBurnTime(ItemType::Coal) == 1600);
    FurnaceRecipeRegistry::initialize(assets, content);

    FurnaceBlockEntity furnace;
    furnace.getSlot(FurnaceBlockEntity::Input) = {BlockType::IronOre};
    furnace.getSlot(FurnaceBlockEntity::Fuel) = {ItemType::Coal, 1};
    for (int tick = 0; tick < FurnaceBlockEntity::COOK_TIME_TICKS; ++tick)
        static_cast<void>(furnace.tick());
    assert(furnace.getSlot(FurnaceBlockEntity::Output).item ==
           ItemType::IronIngot);

    BlockEntityStore entities;
    assert(entities.types().size() == 3);
    ChestBlockEntity& chest = entities.getOrCreateChest({1, 64, 2});
    chest.getSlot(0) = {ItemType::Diamond, 3};
    std::vector<BlockEntityRecord> snapshot = entities.snapshot();
    assert(snapshot.size() == 1);
    assert(snapshot.front().type.toString() == "minecraft:chest");
    BlockEntityStore restored;
    restored.restore(std::move(snapshot));
    assert(restored.getChest({1, 64, 2})->getSlot(0).count == 3);
    SpawnerBlockEntity& spawner = entities.getOrCreateSpawner({4, 20, 9});
    spawner.setMobId(2);
    const std::vector<BlockEntityRecord> spawnerSnapshot = entities.snapshot();
    BlockEntityStore restoredSpawner;
    restoredSpawner.restore(spawnerSnapshot);
    assert(restoredSpawner.getSpawner({4, 20, 9})->mobId() == 2);
}
