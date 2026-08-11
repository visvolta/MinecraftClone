#include "gameplay/GameplayRegistries.h"

#include <initializer_list>
#include <string_view>
#include <utility>

namespace mc::gameplay
{
namespace
{
struct MobProfile
{
    const char* texture = "zombie/zombie";
    MobModelKind model = MobModelKind::Biped;
    SpawnPlacement placement = SpawnPlacement::OnGround;
    float eyeHeight = 1.62f;
    float health = 20.0f;
    float speed = 0.25f;
    float attack = 0.0f;
    float followRange = 16.0f;
    bool burnsInDaylight = false;
    bool fireImmune = false;
};

MobProfile vanillaProfile(std::string_view name)
{
    MobProfile profile;
    if (name == "zombie") profile = {"zombie/zombie", MobModelKind::Biped, SpawnPlacement::OnGround, 1.74f, 20, 0.23f, 3, 35, true, false};
    else if (name == "skeleton") profile = {"skeleton/skeleton", MobModelKind::Skeleton, SpawnPlacement::OnGround, 1.74f, 20, 0.25f, 2, 16, true, false};
    else if (name == "creeper") profile = {"creeper/creeper", MobModelKind::Creeper, SpawnPlacement::OnGround, 1.53f, 20, 0.25f, 0, 16, false, false};
    else if (name == "spider") profile = {"spider/spider", MobModelKind::Spider, SpawnPlacement::OnGround, 0.65f, 16, 0.30f, 2, 16, false, false};
    else if (name == "cow") profile = {"cow/cow", MobModelKind::Cow, SpawnPlacement::OnGround, 1.30f, 10, 0.20f, 0, 16, false, false};
    else if (name == "pig") profile = {"pig/pig", MobModelKind::Pig, SpawnPlacement::OnGround, 0.765f, 10, 0.25f, 0, 16, false, false};
    else if (name == "sheep") profile = {"sheep/sheep", MobModelKind::Sheep, SpawnPlacement::OnGround, 1.235f, 8, 0.23f, 0, 16, false, false};
    else if (name == "chicken") profile = {"chicken", MobModelKind::Chicken, SpawnPlacement::OnGround, 0.644f, 4, 0.25f, 0, 16, false, false};
    else if (name == "bat") profile = {"bat", MobModelKind::Bat, SpawnPlacement::NoRestrictions, 0.45f, 6, 0.10f, 0, 16, false, false};
    else if (name == "blaze") profile = {"blaze", MobModelKind::Blaze, SpawnPlacement::OnGround, 1.53f, 20, 0.23f, 6, 48, false, true};
    else if (name == "cave_spider") profile = {"spider/cave_spider", MobModelKind::Spider, SpawnPlacement::OnGround, 0.45f, 12, 0.30f, 2, 16, false, false};
    else if (name == "donkey") profile = {"horse/donkey", MobModelKind::Horse, SpawnPlacement::OnGround, 1.40f, 15, 0.175f, 0, 16, false, false};
    else if (name == "elder_guardian") profile = {"guardian_elder", MobModelKind::Guardian, SpawnPlacement::InWater, 1.70f, 80, 0.30f, 8, 16, false, false};
    else if (name == "ender_dragon") profile = {"enderdragon/dragon", MobModelKind::Dragon, SpawnPlacement::NoRestrictions, 6.8f, 200, 0.30f, 10, 64, false, false};
    else if (name == "enderman") profile = {"enderman/enderman", MobModelKind::Enderman, SpawnPlacement::OnGround, 2.55f, 40, 0.30f, 7, 64, false, false};
    else if (name == "endermite") profile = {"endermite", MobModelKind::Endermite, SpawnPlacement::OnGround, 0.13f, 8, 0.25f, 2, 64, false, false};
    else if (name == "evocation_illager") profile = {"illager/evoker", MobModelKind::Illager, SpawnPlacement::OnGround, 1.62f, 24, 0.50f, 6, 12, false, false};
    else if (name == "ghast") profile = {"ghast/ghast", MobModelKind::Ghast, SpawnPlacement::NoRestrictions, 2.60f, 10, 0.10f, 6, 100, false, true};
    else if (name == "giant") profile = {"zombie/zombie", MobModelKind::Biped, SpawnPlacement::OnGround, 10.44f, 100, 0.50f, 50, 16, true, false};
    else if (name == "guardian") profile = {"guardian", MobModelKind::Guardian, SpawnPlacement::InWater, 0.425f, 30, 0.50f, 6, 16, false, false};
    else if (name == "horse") profile = {"horse/horse_white", MobModelKind::Horse, SpawnPlacement::OnGround, 1.40f, 22.5f, 0.225f, 0, 16, false, false};
    else if (name == "husk") profile = {"zombie/husk", MobModelKind::Biped, SpawnPlacement::OnGround, 1.74f, 20, 0.23f, 3, 35, false, false};
    else if (name == "iron_golem") profile = {"iron_golem", MobModelKind::IronGolem, SpawnPlacement::OnGround, 2.295f, 100, 0.25f, 15, 16, false, false};
    else if (name == "illusion_illager") profile = {"illager/illusionist", MobModelKind::Illager, SpawnPlacement::OnGround, 1.62f, 32, 0.50f, 5, 18, false, false};
    else if (name == "llama") profile = {"llama/llama_creamy", MobModelKind::Llama, SpawnPlacement::OnGround, 1.7765f, 22, 0.175f, 1, 40, false, false};
    else if (name == "magma_cube") profile = {"slime/magmacube", MobModelKind::MagmaCube, SpawnPlacement::OnGround, 0.325f, 4, 0.40f, 6, 16, false, true};
    else if (name == "mule") profile = {"horse/mule", MobModelKind::Horse, SpawnPlacement::OnGround, 1.40f, 15, 0.175f, 0, 16, false, false};
    else if (name == "mushroom_cow") profile = {"cow/mooshroom", MobModelKind::Cow, SpawnPlacement::OnGround, 1.30f, 10, 0.20f, 0, 16, false, false};
    else if (name == "ocelot") profile = {"cat/ocelot", MobModelKind::Ocelot, SpawnPlacement::OnGround, 0.595f, 10, 0.30f, 3, 16, false, false};
    else if (name == "parrot") profile = {"parrot/parrot_red_blue", MobModelKind::Parrot, SpawnPlacement::NoRestrictions, 0.756f, 6, 0.20f, 0, 16, false, false};
    else if (name == "polar_bear") profile = {"bear/polarbear", MobModelKind::PolarBear, SpawnPlacement::OnGround, 1.19f, 30, 0.25f, 6, 20, false, false};
    else if (name == "rabbit") profile = {"rabbit/brown", MobModelKind::Rabbit, SpawnPlacement::OnGround, 0.425f, 3, 0.30f, 3, 16, false, false};
    else if (name == "shulker") profile = {"shulker/shulker_purple", MobModelKind::Shulker, SpawnPlacement::OnGround, 0.50f, 30, 0.00f, 4, 16, false, false};
    else if (name == "silverfish") profile = {"silverfish", MobModelKind::Silverfish, SpawnPlacement::OnGround, 0.13f, 8, 0.25f, 1, 16, false, false};
    else if (name == "skeleton_horse") profile = {"horse/horse_skeleton", MobModelKind::Horse, SpawnPlacement::OnGround, 1.40f, 15, 0.20f, 0, 16, false, false};
    else if (name == "slime") profile = {"slime/slime", MobModelKind::Slime, SpawnPlacement::OnGround, 0.325f, 4, 0.40f, 4, 16, false, false};
    else if (name == "snowman") profile = {"snowman", MobModelKind::SnowGolem, SpawnPlacement::OnGround, 1.70f, 4, 0.20f, 0, 16, false, false};
    else if (name == "squid") profile = {"squid", MobModelKind::Squid, SpawnPlacement::InWater, 0.40f, 10, 0.70f, 0, 16, false, false};
    else if (name == "stray") profile = {"skeleton/stray", MobModelKind::Skeleton, SpawnPlacement::OnGround, 1.74f, 20, 0.25f, 2, 16, true, false};
    else if (name == "vex") profile = {"illager/vex", MobModelKind::Vex, SpawnPlacement::NoRestrictions, 0.51875f, 14, 0.70f, 4, 16, false, false};
    else if (name == "villager") profile = {"villager/villager", MobModelKind::Villager, SpawnPlacement::OnGround, 1.62f, 20, 0.50f, 0, 16, false, false};
    else if (name == "vindication_illager") profile = {"illager/vindicator", MobModelKind::Illager, SpawnPlacement::OnGround, 1.62f, 24, 0.35f, 5, 12, false, false};
    else if (name == "witch") profile = {"witch", MobModelKind::Witch, SpawnPlacement::OnGround, 1.62f, 26, 0.25f, 0, 16, false, false};
    else if (name == "wither_skeleton") profile = {"skeleton/wither_skeleton", MobModelKind::Skeleton, SpawnPlacement::OnGround, 2.10f, 20, 0.25f, 4, 16, false, true};
    else if (name == "wolf") profile = {"wolf/wolf", MobModelKind::Wolf, SpawnPlacement::OnGround, 0.68f, 8, 0.30f, 2, 16, false, false};
    else if (name == "zombie_horse") profile = {"horse/horse_zombie", MobModelKind::Horse, SpawnPlacement::OnGround, 1.40f, 15, 0.20f, 0, 16, false, false};
    else if (name == "zombie_pigman") profile = {"zombie_pigman", MobModelKind::Biped, SpawnPlacement::OnGround, 1.74f, 20, 0.23f, 5, 16, false, true};
    else if (name == "zombie_villager") profile = {"zombie_villager/zombie_villager", MobModelKind::ZombieVillager, SpawnPlacement::OnGround, 1.74f, 20, 0.23f, 3, 35, true, false};
    else if (name == "wither") profile = {"wither/wither", MobModelKind::Wither, SpawnPlacement::NoRestrictions, 3.00f, 300, 0.60f, 8, 40, false, true};
    return profile;
}

std::uint64_t goals(std::initializer_list<MobAiGoal> values)
{
    std::uint64_t result = 0;
    for (const MobAiGoal value : values)
        result |= static_cast<std::uint64_t>(value);
    return result;
}

void configureVanillaAi(std::string_view name, MobDefinition& definition)
{
    const auto textures = [](std::initializer_list<const char*> paths)
    {
        std::vector<core::ResourceLocation> result;
        result.reserve(paths.size());
        for (const char* path : paths)
            result.emplace_back("minecraft", std::string("entity/") + path);
        return result;
    };
    const auto passive = goals({
        MobAiGoal::Swim, MobAiGoal::Panic, MobAiGoal::Mate,
        MobAiGoal::Tempt, MobAiGoal::FollowParent, MobAiGoal::Wander,
        MobAiGoal::WatchPlayer, MobAiGoal::LookIdle
    });
    const auto hostile = goals({
        MobAiGoal::Swim, MobAiGoal::AttackPlayer, MobAiGoal::HurtByTarget,
        MobAiGoal::Wander, MobAiGoal::WatchPlayer, MobAiGoal::LookIdle
    });
    definition.aiGoals = definition.category == MobCategory::Monster
        ? hostile : passive;
    definition.attackKind = definition.attackDamage > 0.0f
        ? MobAttackKind::Melee : MobAttackKind::None;
    definition.movementKind = MobMovementKind::Ground;

    if (name == "zombie_pigman" || name == "enderman")
        definition.aiGoals &= ~static_cast<std::uint64_t>(
            MobAiGoal::AttackPlayer
        );

    if (definition.spawnPlacement == SpawnPlacement::NoRestrictions)
        definition.movementKind = MobMovementKind::Flying;
    else if (definition.spawnPlacement == SpawnPlacement::InWater)
        definition.movementKind = MobMovementKind::Aquatic;
    if (name == "blaze")
        definition.movementKind = MobMovementKind::Flying;

    if (name == "skeleton" || name == "stray" ||
        name == "illusion_illager")
    {
        definition.attackKind = MobAttackKind::Ranged;
        definition.attackRange = 15.0f;
        definition.attackIntervalTicks = 20;
        definition.aiGoals |= static_cast<std::uint64_t>(
            MobAiGoal::RestrictSun
        ) | static_cast<std::uint64_t>(MobAiGoal::AvoidSun);
    }
    else if (name == "creeper")
    {
        definition.attackKind = MobAttackKind::CreeperExplosion;
        definition.attackRange = 3.0f;
        definition.attackIntervalTicks = 30;
    }
    else if (name == "witch" || name == "blaze" || name == "ghast" ||
             name == "guardian" || name == "elder_guardian" ||
             name == "wither" || name == "ender_dragon" ||
             name == "snowman")
    {
        definition.attackKind = MobAttackKind::Ranged;
        definition.attackRange = name == "ghast" ? 64.0f : 16.0f;
        definition.attackIntervalTicks = name == "blaze" ? 30 : 40;
    }

    if (name == "rabbit" || name == "slime" || name == "magma_cube")
        definition.movementKind = MobMovementKind::Hopping;
    if (name == "zombie" || name == "zombie_villager" ||
        name == "skeleton")
    {
        definition.aiGoals |= static_cast<std::uint64_t>(
            MobAiGoal::MoveIndoors
        ) | static_cast<std::uint64_t>(MobAiGoal::RestrictSun);
    }
    if (name == "spider" || name == "cave_spider")
        definition.aiGoals |= static_cast<std::uint64_t>(
            MobAiGoal::LeapAtTarget
        );
    if (name == "ocelot" || name == "rabbit")
        definition.aiGoals |= static_cast<std::uint64_t>(
            MobAiGoal::AvoidPlayer
        );
    if (name == "wolf")
        definition.aiGoals |= goals({
            MobAiGoal::Sit, MobAiGoal::FollowOwner, MobAiGoal::Beg,
            MobAiGoal::NearestHostileTarget
        });
    if (name == "parrot")
        definition.aiGoals |= static_cast<std::uint64_t>(
            MobAiGoal::LandOnOwnersShoulder
        );
    if (name == "sheep")
    {
        definition.aiGoals |= static_cast<std::uint64_t>(MobAiGoal::EatGrass);
        definition.overlayTexture = core::ResourceLocation(
            "minecraft:entity/sheep/sheep_fur"
        );
    }
    if (name == "spider" || name == "cave_spider")
        definition.overlayTexture = core::ResourceLocation(
            "minecraft:entity/spider_eyes"
        );
    if (name == "enderman")
        definition.overlayTexture = core::ResourceLocation(
            "minecraft:entity/enderman/enderman_eyes"
        );
    if (name == "stray")
        definition.overlayTexture = core::ResourceLocation(
            "minecraft:entity/skeleton/stray_overlay"
        );

    if (name == "giant") definition.renderScale = 6.0f;
    else if (name == "cave_spider") definition.renderScale = 0.7f;
    else if (name == "wither_skeleton") definition.renderScale = 1.2f;
    else if (name == "husk") definition.renderScale = 1.0625f;
    else if (name == "elder_guardian") definition.renderScale = 2.35f;

    if (name == "villager")
    {
        definition.aiGoals = goals({
            MobAiGoal::Swim, MobAiGoal::AvoidPlayer, MobAiGoal::TradePlayer,
            MobAiGoal::LookIdle, MobAiGoal::MoveIndoors, MobAiGoal::OpenDoor,
            MobAiGoal::MoveVillage, MobAiGoal::Mate, MobAiGoal::Wander,
            MobAiGoal::WatchPlayer
        });
        definition.variantTextures = textures({
            "villager/villager", "villager/farmer", "villager/librarian",
            "villager/priest", "villager/smith", "villager/butcher"
        });
    }
    if (name == "zombie_villager")
        definition.variantTextures = textures({
            "zombie_villager/zombie_villager",
            "zombie_villager/zombie_farmer",
            "zombie_villager/zombie_librarian",
            "zombie_villager/zombie_priest",
            "zombie_villager/zombie_smith",
            "zombie_villager/zombie_butcher"
        });
    if (name == "horse")
    {
        definition.variantTextures = textures({
            "horse/horse_white", "horse/horse_creamy",
            "horse/horse_chestnut", "horse/horse_brown",
            "horse/horse_black", "horse/horse_gray",
            "horse/horse_darkbrown"
        });
        definition.variantOverlayTextures = textures({
            "horse/horse_markings_white",
            "horse/horse_markings_whitefield",
            "horse/horse_markings_whitedots",
            "horse/horse_markings_blackdots"
        });
    }
    if (name == "rabbit")
        definition.variantTextures = textures({
            "rabbit/brown", "rabbit/white", "rabbit/black",
            "rabbit/white_splotched", "rabbit/gold", "rabbit/salt"
        });
    if (name == "llama")
        definition.variantTextures = textures({
            "llama/llama_creamy", "llama/llama_white",
            "llama/llama_brown", "llama/llama_gray"
        });
    if (name == "parrot")
        definition.variantTextures = textures({
            "parrot/parrot_red_blue", "parrot/parrot_blue",
            "parrot/parrot_green", "parrot/parrot_yellow_blue",
            "parrot/parrot_grey"
        });
}
}

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
        const MobProfile profile = vanillaProfile(name);
        MobDefinition definition{
            display, category, width, height, 80,
            weight, minGroup, maxGroup,
            core::ResourceLocation("minecraft", std::string("entities/") + name),
            core::ResourceLocation("minecraft", std::string("entity/") + profile.texture),
            profile.model, profile.placement, profile.eyeHeight,
            profile.health, profile.speed, profile.attack,
            profile.followRange, profile.burnsInDaylight,
            profile.fireImmune, MobMovementKind::Ground,
            MobAttackKind::None,
            goals({MobAiGoal::Wander, MobAiGoal::WatchPlayer,
                   MobAiGoal::LookIdle}),
            1.5f, 20, core::ResourceLocation("minecraft:entity/empty"),
            1.0f, {}, {}
        };
        configureVanillaAi(name, definition);
        registries.registerMob(
            core::ResourceLocation("minecraft", name), std::move(definition)
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
    mob("bat", "Bat", MobCategory::Ambient, 0.5f, 0.9f, 10, 8, 8);
    mob("blaze", "Blaze", MobCategory::Monster, 0.6f, 1.8f, 10, 2, 3);
    mob("cave_spider", "Cave Spider", MobCategory::Monster, 0.7f, 0.5f, 100, 4, 4);
    mob("donkey", "Donkey", MobCategory::Creature, 1.3965f, 1.6f, 1, 2, 6);
    mob("elder_guardian", "Elder Guardian", MobCategory::Monster, 1.9975f, 1.9975f, 0, 1, 1);
    mob("ender_dragon", "Ender Dragon", MobCategory::Monster, 16.0f, 8.0f, 0, 1, 1);
    mob("enderman", "Enderman", MobCategory::Monster, 0.6f, 2.9f, 10, 1, 4);
    mob("endermite", "Endermite", MobCategory::Monster, 0.4f, 0.3f, 0, 1, 1);
    mob("evocation_illager", "Evoker", MobCategory::Monster, 0.6f, 1.95f, 0, 1, 1);
    mob("ghast", "Ghast", MobCategory::Monster, 4.0f, 4.0f, 50, 4, 4);
    mob("giant", "Giant", MobCategory::Monster, 3.6f, 12.0f, 0, 1, 1);
    mob("guardian", "Guardian", MobCategory::Monster, 0.85f, 0.85f, 0, 2, 4);
    mob("horse", "Horse", MobCategory::Creature, 1.3965f, 1.6f, 5, 2, 6);
    mob("husk", "Husk", MobCategory::Monster, 0.6f, 1.95f, 80, 4, 4);
    mob("iron_golem", "Iron Golem", MobCategory::Creature, 1.4f, 2.7f, 0, 1, 1);
    mob("illusion_illager", "Illusioner", MobCategory::Monster, 0.6f, 1.95f, 0, 1, 1);
    mob("llama", "Llama", MobCategory::Creature, 0.9f, 1.87f, 5, 4, 6);
    mob("magma_cube", "Magma Cube", MobCategory::Monster, 0.52f, 0.52f, 2, 4, 4);
    mob("mule", "Mule", MobCategory::Creature, 1.3965f, 1.6f, 0, 1, 1);
    mob("mushroom_cow", "Mooshroom", MobCategory::Creature, 0.9f, 1.4f, 8, 4, 8);
    mob("ocelot", "Ocelot", MobCategory::Creature, 0.6f, 0.7f, 2, 1, 1);
    mob("parrot", "Parrot", MobCategory::Creature, 0.5f, 0.9f, 40, 1, 2);
    mob("polar_bear", "Polar Bear", MobCategory::Creature, 1.3f, 1.4f, 1, 1, 2);
    mob("rabbit", "Rabbit", MobCategory::Creature, 0.4f, 0.5f, 4, 2, 3);
    mob("shulker", "Shulker", MobCategory::Monster, 1.0f, 1.0f, 0, 1, 1);
    mob("silverfish", "Silverfish", MobCategory::Monster, 0.4f, 0.3f, 10, 4, 4);
    mob("skeleton_horse", "Skeleton Horse", MobCategory::Creature, 1.3965f, 1.6f, 0, 1, 1);
    mob("slime", "Slime", MobCategory::Monster, 0.52f, 0.52f, 100, 4, 4);
    mob("snowman", "Snow Golem", MobCategory::Creature, 0.7f, 1.9f, 0, 1, 1);
    mob("squid", "Squid", MobCategory::WaterCreature, 0.8f, 0.8f, 10, 1, 4);
    mob("stray", "Stray", MobCategory::Monster, 0.6f, 1.99f, 80, 4, 4);
    mob("vex", "Vex", MobCategory::Monster, 0.4f, 0.8f, 0, 1, 1);
    mob("villager", "Villager", MobCategory::Creature, 0.6f, 1.95f, 0, 1, 1);
    mob("vindication_illager", "Vindicator", MobCategory::Monster, 0.6f, 1.95f, 0, 1, 1);
    mob("witch", "Witch", MobCategory::Monster, 0.6f, 1.95f, 5, 1, 1);
    mob("wither_skeleton", "Wither Skeleton", MobCategory::Monster, 0.7f, 2.4f, 10, 5, 5);
    mob("wolf", "Wolf", MobCategory::Creature, 0.6f, 0.85f, 5, 4, 4);
    mob("zombie_horse", "Zombie Horse", MobCategory::Creature, 1.3965f, 1.6f, 0, 1, 1);
    mob("zombie_pigman", "Zombie Pigman", MobCategory::Monster, 0.6f, 1.95f, 100, 4, 4);
    mob("zombie_villager", "Zombie Villager", MobCategory::Monster, 0.6f, 1.95f, 5, 1, 1);

    MobDefinition wither{
        "Wither", MobCategory::Monster, 0.9f, 3.5f, 80,
        0, 1, 1, core::ResourceLocation("minecraft:entities/wither"),
        core::ResourceLocation("minecraft:entity/wither/wither"),
        MobModelKind::Wither, SpawnPlacement::NoRestrictions,
        3.0f, 300.0f, 0.6f, 8.0f, 40.0f, false, true,
        MobMovementKind::Flying, MobAttackKind::Ranged,
        goals({MobAiGoal::AttackPlayer, MobAiGoal::HurtByTarget,
               MobAiGoal::Wander}),
        16.0f, 40, core::ResourceLocation("minecraft:entity/empty"),
        1.0f, {}, {}
    };
    configureVanillaAi("wither", wither);
    registries.registerMob(
        core::ResourceLocation("minecraft:wither"), std::move(wither)
    );

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
    structure("temple", "Temple", 32, 8, 14357617U);
    structure("stronghold", "Stronghold", 32, 8, 0x5706U);
    structure("ocean_monument", "Ocean Monument", 32, 5, 10387313U);
    structure("woodland_mansion", "Woodland Mansion", 80, 20, 10387319U);
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
