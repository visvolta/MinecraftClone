#pragma once

#include "Item.h"
#include "core/Registry.h"

#include <cstdint>
#include <string>
#include <vector>

namespace mc::gameplay
{
struct DimensionDefinition
{
    int legacyId = 0;
    std::string displayName;
    int minimumY = 0;
    int height = 256;
    double coordinateScale = 1.0;
    bool hasSkyLight = true;
    bool ultrawarm = false;
    bool natural = true;
};

enum class MobCategory : std::uint8_t
{
    Monster,
    Creature,
    Ambient,
    WaterCreature
};

enum class MobModelKind : std::uint8_t
{
    Biped,
    Skeleton,
    Enderman,
    ZombieVillager,
    Quadruped,
    Cow,
    Pig,
    Sheep,
    Spider,
    Creeper,
    Chicken,
    Bat,
    Blaze,
    Guardian,
    Dragon,
    Ghast,
    Horse,
    IronGolem,
    Llama,
    Ocelot,
    Parrot,
    PolarBear,
    Rabbit,
    Shulker,
    Silverfish,
    Endermite,
    Slime,
    MagmaCube,
    Squid,
    Villager,
    Illager,
    Witch,
    Wither,
    Wolf,
    SnowGolem,
    Vex,
    Count
};

enum class SpawnPlacement : std::uint8_t
{
    OnGround,
    InWater,
    NoRestrictions
};

enum class MobMovementKind : std::uint8_t
{
    Ground,
    Flying,
    Aquatic,
    Hopping
};

enum class MobAttackKind : std::uint8_t
{
    None,
    Melee,
    Ranged,
    CreeperExplosion
};

enum class MobGoalKind : std::uint8_t
{
    Swim,
    Panic,
    Mate,
    Tempt,
    FollowParent,
    WanderAvoidWater,
    WatchPlayer,
    LookIdle,
    EatGrass,
    AvoidPlayer,
    Sit,
    FollowOwner,
    Beg,
    LeapAtTarget,
    MeleeAttack,
    NearestPlayerTarget,
    HurtByTarget,
    RestrictSun,
    FleeSun,
    CreeperSwell,
    RangedAttack,
    LandOnOwnerShoulder,
    RunAroundLikeCrazy,
    FollowCaravan
};

struct MobGoalDefinition
{
    MobGoalKind kind = MobGoalKind::WanderAvoidWater;
    int priority = 0;
    std::uint8_t mutexBits = 0;
    double speed = 1.0;
    float range = 0.0f;
    float stopRange = 0.0f;
    float chance = 0.0f;
    std::vector<ItemType> items;
};

enum class AnimalKind : std::uint8_t
{
    None,
    Cow,
    Pig,
    Sheep,
    Chicken,
    Rabbit,
    Horse,
    Donkey,
    Mule,
    Mooshroom,
    Llama,
    Wolf,
    Ocelot,
    PolarBear
};

enum class TameableKind : std::uint8_t
{
    None,
    Wolf,
    Ocelot,
    Parrot,
    Horse,
    Donkey,
    Mule,
    Llama,
    SkeletonHorse,
    ZombieHorse
};

enum class MobAiGoal : std::uint64_t
{
    None = 0,
    Swim = 1ULL << 0,
    AvoidSun = 1ULL << 1,
    RestrictSun = 1ULL << 2,
    Panic = 1ULL << 3,
    Mate = 1ULL << 4,
    Tempt = 1ULL << 5,
    FollowParent = 1ULL << 6,
    Wander = 1ULL << 7,
    WatchPlayer = 1ULL << 8,
    LookIdle = 1ULL << 9,
    AttackPlayer = 1ULL << 10,
    HurtByTarget = 1ULL << 11,
    LeapAtTarget = 1ULL << 12,
    AvoidPlayer = 1ULL << 13,
    MoveVillage = 1ULL << 14,
    MoveIndoors = 1ULL << 15,
    OpenDoor = 1ULL << 16,
    TradePlayer = 1ULL << 17,
    Sit = 1ULL << 18,
    FollowOwner = 1ULL << 19,
    Beg = 1ULL << 20,
    NearestHostileTarget = 1ULL << 21,
    LandOnOwnersShoulder = 1ULL << 22,
    EatGrass = 1ULL << 23
};

[[nodiscard]] constexpr std::uint64_t operator|(
    MobAiGoal left,
    MobAiGoal right) noexcept
{
    return static_cast<std::uint64_t>(left) |
           static_cast<std::uint64_t>(right);
}

[[nodiscard]] constexpr std::uint64_t operator|(
    std::uint64_t left,
    MobAiGoal right) noexcept
{
    return left | static_cast<std::uint64_t>(right);
}

[[nodiscard]] constexpr bool hasAiGoal(
    std::uint64_t goals,
    MobAiGoal goal) noexcept
{
    return (goals & static_cast<std::uint64_t>(goal)) != 0;
}

struct MobDefinition
{
    std::string displayName;
    MobCategory category = MobCategory::Creature;
    float width = 0.6f;
    float height = 1.8f;
    int trackingRange = 80;
    int spawnWeight = 10;
    int minimumGroup = 1;
    int maximumGroup = 4;
    core::ResourceLocation lootTable{"minecraft:entities/empty"};
    core::ResourceLocation texture{"minecraft:entity/zombie/zombie"};
    MobModelKind model = MobModelKind::Biped;
    SpawnPlacement spawnPlacement = SpawnPlacement::OnGround;
    float eyeHeight = 1.62f;
    float maximumHealth = 20.0f;
    float movementSpeed = 0.25f;
    float attackDamage = 0.0f;
    float followRange = 16.0f;
    bool burnsInDaylight = false;
    bool fireImmune = false;
    MobMovementKind movementKind = MobMovementKind::Ground;
    MobAttackKind attackKind = MobAttackKind::None;
    std::uint64_t aiGoals =
        MobAiGoal::Wander | MobAiGoal::WatchPlayer | MobAiGoal::LookIdle;
    float attackRange = 1.5f;
    int attackIntervalTicks = 20;
    core::ResourceLocation overlayTexture{"minecraft:entity/empty"};
    float renderScale = 1.0f;
    std::vector<core::ResourceLocation> variantTextures;
    std::vector<core::ResourceLocation> variantOverlayTextures;
    float stepHeight = 0.6f;
    int maximumFallHeight = 3;
    bool ageable = false;
    bool breedable = false;
    AnimalKind animalKind = AnimalKind::None;
    TameableKind tameableKind = TameableKind::None;
    std::vector<ItemType> breedingItems;
    std::vector<ItemType> tamingItems;
    std::vector<MobGoalDefinition> goalTasks;
    std::vector<MobGoalDefinition> targetGoalTasks;
    int maximumTemper = 0;
};

struct StructureDefinition
{
    std::string displayName;
    int spacing = 32;
    int separation = 8;
    std::uint32_t salt = 0;
    bool terrainAdapted = true;
};

struct AdvancementDefinition
{
    std::string displayName;
    core::ResourceLocation parent{"minecraft:root"};
    core::ResourceLocation trigger{"minecraft:impossible"};
};

class GameplayRegistries
{
public:
    GameplayRegistries();

    DimensionDefinition& registerDimension(
        core::ResourceLocation name,
        DimensionDefinition definition
    );
    MobDefinition& registerMob(
        core::ResourceLocation name,
        MobDefinition definition
    );
    StructureDefinition& registerStructure(
        core::ResourceLocation name,
        StructureDefinition definition
    );
    AdvancementDefinition& registerAdvancement(
        core::ResourceLocation name,
        AdvancementDefinition definition
    );
    void freeze();

    [[nodiscard]] const core::Registry<DimensionDefinition>& dimensions() const noexcept;
    [[nodiscard]] const core::Registry<MobDefinition>& mobs() const noexcept;
    [[nodiscard]] const core::Registry<StructureDefinition>& structures() const noexcept;
    [[nodiscard]] const core::Registry<AdvancementDefinition>& advancements() const noexcept;

private:
    core::Registry<DimensionDefinition> dimensions_;
    core::Registry<MobDefinition> mobs_;
    core::Registry<StructureDefinition> structures_;
    core::Registry<AdvancementDefinition> advancements_;
};

void registerVanillaGameplay(GameplayRegistries& registries);
}
