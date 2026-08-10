#include "gameplay/GameplayRegistries.h"

namespace mc::gameplay
{
GameplayRegistries::GameplayRegistries()
    : dimensions_(core::ResourceLocation("minecraft:dimensions")),
      mobs_(core::ResourceLocation("minecraft:mob_types")),
      structures_(core::ResourceLocation("minecraft:structures")),
      advancements_(core::ResourceLocation("minecraft:advancements"))
{
}

DimensionDefinition& GameplayRegistries::registerDimension(
    core::ResourceLocation name,
    DimensionDefinition definition)
{
    return dimensions_.registerValue(std::move(name), std::move(definition));
}

MobDefinition& GameplayRegistries::registerMob(
    core::ResourceLocation name,
    MobDefinition definition)
{
    return mobs_.registerValue(std::move(name), std::move(definition));
}

StructureDefinition& GameplayRegistries::registerStructure(
    core::ResourceLocation name,
    StructureDefinition definition)
{
    return structures_.registerValue(std::move(name), std::move(definition));
}

AdvancementDefinition& GameplayRegistries::registerAdvancement(
    core::ResourceLocation name,
    AdvancementDefinition definition)
{
    return advancements_.registerValue(std::move(name), std::move(definition));
}

void GameplayRegistries::freeze()
{
    dimensions_.freeze();
    mobs_.freeze();
    structures_.freeze();
    advancements_.freeze();
}

const core::Registry<DimensionDefinition>&
GameplayRegistries::dimensions() const noexcept { return dimensions_; }
const core::Registry<MobDefinition>&
GameplayRegistries::mobs() const noexcept { return mobs_; }
const core::Registry<StructureDefinition>&
GameplayRegistries::structures() const noexcept { return structures_; }
const core::Registry<AdvancementDefinition>&
GameplayRegistries::advancements() const noexcept { return advancements_; }

void registerVanillaGameplay(GameplayRegistries& registries)
{
    registries.registerDimension(
        core::ResourceLocation("minecraft:overworld"),
        {0, "Overworld", 0, 256, 1.0, true, false, true}
    );
    registries.registerDimension(
        core::ResourceLocation("minecraft:the_nether"),
        {-1, "The Nether", 0, 128, 8.0, false, true, false}
    );
    registries.registerDimension(
        core::ResourceLocation("minecraft:the_end"),
        {1, "The End", 0, 256, 1.0, false, false, false}
    );

    const auto mob = [&registries](
        const char* name, const char* display, MobCategory category,
        float width, float height, int weight, int minGroup, int maxGroup)
    {
        registries.registerMob(
            core::ResourceLocation("minecraft", name),
            {display, category, width, height, 80, weight, minGroup, maxGroup}
        );
    };
    mob("zombie", "Zombie", MobCategory::Monster, 0.6f, 1.95f, 100, 4, 4);
    mob("skeleton", "Skeleton", MobCategory::Monster, 0.6f, 1.99f, 100, 4, 4);
    mob("creeper", "Creeper", MobCategory::Monster, 0.6f, 1.7f, 100, 4, 4);
    mob("spider", "Spider", MobCategory::Monster, 1.4f, 0.9f, 100, 4, 4);
    mob("cow", "Cow", MobCategory::Creature, 0.9f, 1.4f, 8, 4, 4);
    mob("pig", "Pig", MobCategory::Creature, 0.9f, 0.9f, 10, 4, 4);
    mob("sheep", "Sheep", MobCategory::Creature, 0.9f, 1.3f, 12, 4, 4);
    mob("chicken", "Chicken", MobCategory::Creature, 0.4f, 0.7f, 10, 4, 4);

    const auto structure = [&registries](
        const char* name, const char* display, int spacing,
        int separation, std::uint32_t salt)
    {
        registries.registerStructure(
            core::ResourceLocation("minecraft", name),
            {display, spacing, separation, salt, true}
        );
    };
    structure("dungeon", "Dungeon", 8, 0, 0xD06E0U);
    structure("mineshaft", "Mineshaft", 1, 0, 0x5A17U);
    structure("village", "Village", 32, 8, 10387312U);
    structure("stronghold", "Stronghold", 32, 8, 0x5706U);
    structure("nether_fortress", "Nether Fortress", 27, 4, 30084232U);
    structure("end_city", "End City", 20, 11, 10387313U);

    registries.registerAdvancement(
        core::ResourceLocation("minecraft:story/root"),
        {"Minecraft", core::ResourceLocation("minecraft:root"),
         core::ResourceLocation("minecraft:tick")}
    );
    registries.registerAdvancement(
        core::ResourceLocation("minecraft:story/mine_stone"),
        {"Stone Age", core::ResourceLocation("minecraft:story/root"),
         core::ResourceLocation("minecraft:inventory_changed")}
    );
    registries.registerAdvancement(
        core::ResourceLocation("minecraft:story/smelt_iron"),
        {"Acquire Hardware", core::ResourceLocation("minecraft:story/mine_stone"),
         core::ResourceLocation("minecraft:inventory_changed")}
    );
}
}
