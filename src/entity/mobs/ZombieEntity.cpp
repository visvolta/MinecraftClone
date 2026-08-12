#include "entity/mobs/ZombieEntity.h"

#include "World.h"
#include "entity/Math.h"
#include "entity/ai/VanillaGoals.h"

namespace mc::entity
{
namespace
{
const EntityUuid BabySpeedId{0xB9766B59ULL, 0x9566CE27ULL};
}

ZombieEntity::ZombieEntity(World& world) : MonsterEntity(world)
{
    setSize(0.6f, 1.95f);
    experienceValue_ = 5;
    undead_ = true;
}

void ZombieEntity::initEntityAI()
{
    tasks_.add(0, std::make_unique<ai::SwimGoal>(*this));
    tasks_.add(2, std::make_unique<ai::AttackMeleeGoal>(*this, 1.0, false));
    tasks_.add(5, std::make_unique<ai::MoveTowardsRestrictionGoal>(*this, 1.0));
    tasks_.add(7, std::make_unique<ai::WanderAvoidWaterGoal>(*this, 1.0));
    tasks_.add(8, std::make_unique<ai::WatchClosestGoal>(*this, 8.0f));
    tasks_.add(8, std::make_unique<ai::LookIdleGoal>(*this));
    applyEntityAI();
}

void ZombieEntity::applyEntityAI()
{
    targetTasks_.add(1, std::make_unique<ai::HurtByTargetGoal>(*this, true));
    targetTasks_.add(2, std::make_unique<ai::NearestAttackableTargetGoal>(*this, true));
}

void ZombieEntity::applyEntityAttributes()
{
    MonsterEntity::applyEntityAttributes();
    getEntityAttribute(SharedMonsterAttributes::FOLLOW_RANGE).setBaseValue(35.0);
    getEntityAttribute(SharedMonsterAttributes::MOVEMENT_SPEED)
        .setBaseValue(0.23000000417232513);
    getEntityAttribute(SharedMonsterAttributes::ATTACK_DAMAGE).setBaseValue(3.0);
    getEntityAttribute(SharedMonsterAttributes::ARMOR).setBaseValue(2.0);
}

void ZombieEntity::onLivingUpdate()
{
    if (world_->isDaytime() && !isChild() && shouldBurnInDay())
    {
        const float brightness = getBrightness();
        if (brightness > 0.5f &&
            rand_.nextFloat() * 30.0f < (brightness - 0.4f) * 2.0f &&
            world_->canSeeSky(
                floorInt(posX),
                floorInt(posY + getEyeHeight()),
                floorInt(posZ)))
        {
            setFire(8);
        }
    }
    MonsterEntity::onLivingUpdate();
}

void ZombieEntity::setChild(bool child)
{
    child_ = child;
    AttributeInstance& speed =
        getEntityAttribute(SharedMonsterAttributes::MOVEMENT_SPEED);
    speed.removeModifier(BabySpeedId);
    if (child)
    {
        setSize(0.3f, 0.975f);
        speed.applyModifier(AttributeModifier(
            BabySpeedId, "Baby speed boost", 0.5,
            AttributeModifier::Operation::MultiplyBase, false));
    }
    else
    {
        setSize(0.6f, 1.95f);
    }
}

void ZombieEntity::onInitialSpawn()
{
    MonsterEntity::onInitialSpawn();
    if (world_->getDifficulty() == Difficulty::Hard && rand_.nextFloat() < 0.05f)
        setChild(true);
}

float ZombieEntity::getEyeHeight() const
{
    return isChild() ? 0.93f : 1.74f;
}

int ZombieEntity::getExperiencePoints(PlayerEntity* player) const
{
    int value = experienceValue_;
    if (isChild())
        value = static_cast<int>(static_cast<float>(value) * 2.5f);
    return value;
}

core::ResourceLocation ZombieEntity::getType() const
{
    return core::ResourceLocation("minecraft:zombie");
}

core::ResourceLocation ZombieEntity::getLootTable() const
{
    return core::ResourceLocation("minecraft:entities/zombie");
}

gameplay::MobModelKind ZombieEntity::getModelKind() const
{
    return gameplay::MobModelKind::Biped;
}

core::ResourceLocation ZombieEntity::getTexture() const
{
    return core::ResourceLocation("minecraft:entity/zombie/zombie");
}

HuskEntity::HuskEntity(World& world) : ZombieEntity(world) {}

core::ResourceLocation HuskEntity::getType() const
{
    return core::ResourceLocation("minecraft:husk");
}

core::ResourceLocation HuskEntity::getLootTable() const
{
    return core::ResourceLocation("minecraft:entities/husk");
}

core::ResourceLocation HuskEntity::getTexture() const
{
    return core::ResourceLocation("minecraft:entity/zombie/husk");
}

ZombieVillagerEntity::ZombieVillagerEntity(World& world) : ZombieEntity(world) {}

core::ResourceLocation ZombieVillagerEntity::getType() const
{
    return core::ResourceLocation("minecraft:zombie_villager");
}

core::ResourceLocation ZombieVillagerEntity::getLootTable() const
{
    return core::ResourceLocation("minecraft:entities/zombie_villager");
}

gameplay::MobModelKind ZombieVillagerEntity::getModelKind() const
{
    return gameplay::MobModelKind::ZombieVillager;
}

core::ResourceLocation ZombieVillagerEntity::getTexture() const
{
    return core::ResourceLocation("minecraft:entity/zombie_villager/zombie_villager");
}
}
