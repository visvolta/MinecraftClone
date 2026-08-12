#include "entity/mobs/TameableMobs.h"

#include "World.h"
#include "entity/Math.h"
#include "entity/PlayerEntity.h"
#include "entity/ai/VanillaGoals.h"
#include "entity/projectile/ArrowEntity.h"

#include <cmath>

namespace mc::entity
{
WolfEntity::WolfEntity(World& world) : TameableEntity(world)
{
    setSize(0.6f, 0.85f);
}

void WolfEntity::initEntityAI()
{
    tasks_.add(1, std::make_unique<ai::SwimGoal>(*this));
    tasks_.add(2, std::make_unique<ai::SitGoal>(*this));
    tasks_.add(4, std::make_unique<ai::LeapAtTargetGoal>(*this, 0.4f));
    tasks_.add(5, std::make_unique<ai::AttackMeleeGoal>(*this, 1.0, true));
    tasks_.add(6, std::make_unique<ai::FollowOwnerGoal>(*this, 1.0, 10.0f, 2.0f));
    tasks_.add(7, std::make_unique<ai::MateGoal>(*this, 1.0));
    tasks_.add(8, std::make_unique<ai::WanderAvoidWaterGoal>(*this, 1.0));
    tasks_.add(9, std::make_unique<ai::BegGoal>(*this, 8.0f));
    tasks_.add(10, std::make_unique<ai::WatchClosestGoal>(*this, 8.0f));
    tasks_.add(10, std::make_unique<ai::LookIdleGoal>(*this));
    targetTasks_.add(3, std::make_unique<ai::HurtByTargetGoal>(*this, true));
}

void WolfEntity::applyEntityAttributes()
{
    TameableEntity::applyEntityAttributes();
    getEntityAttribute(SharedMonsterAttributes::MOVEMENT_SPEED)
        .setBaseValue(0.30000001192092896);
    getEntityAttribute(SharedMonsterAttributes::MAX_HEALTH)
        .setBaseValue(tamed_ ? 20.0 : 8.0);
    attributes_.registerAttribute(SharedMonsterAttributes::ATTACK_DAMAGE).setBaseValue(2.0);
}

void WolfEntity::setTamed(bool tamed)
{
    TameableEntity::setTamed(tamed);
    getEntityAttribute(SharedMonsterAttributes::MAX_HEALTH)
        .setBaseValue(tamed ? 20.0 : 8.0);
    getEntityAttribute(SharedMonsterAttributes::ATTACK_DAMAGE)
        .setBaseValue(tamed ? 4.0 : 2.0);
    if (tamed)
        setHealth(getMaxHealth());
}

void WolfEntity::setAttackTarget(LivingEntity* target)
{
    Mob::setAttackTarget(target);
}

bool WolfEntity::attackEntityAsMob(Entity& target)
{
    const float damage = static_cast<float>(
        getEntityAttribute(SharedMonsterAttributes::ATTACK_DAMAGE).getAttributeValue());
    return target.attackEntityFrom(DamageSource::causeMobDamage(*this), damage);
}

bool WolfEntity::isBreedingItem(ItemType item) const
{
    return item == ItemType::RawBeef || item == ItemType::CookedBeef ||
           item == ItemType::RawChicken || item == ItemType::CookedChicken ||
           item == ItemType::RawPorkchop || item == ItemType::CookedPorkchop ||
           item == ItemType::RottenFlesh;
}

bool WolfEntity::isBegging() const
{
    return false;
}

bool WolfEntity::processInteract(PlayerEntity& player, ItemStack& stack)
{
    if (isTamed())
    {
        if (isBreedingItem(stack.item) && getHealth() < getMaxHealth())
        {
            heal(static_cast<float>(std::max(1, getItemProperties(stack.item).foodPoints)));
            if (stack.count > 1) --stack.count;
            else stack.clear();
            return true;
        }
        if (isOwner(player) && !isBreedingItem(stack.item))
        {
            setSitting(!isSitting());
            getNavigator().clear();
            return true;
        }
    }
    else if (stack.item == ItemType::Bone)
    {
        if (stack.count > 1) --stack.count;
        else stack.clear();
        if (rand_.nextInt(3) == 0)
        {
            setTamed(true);
            setOwnerId(player.uuid());
            setSitting(true);
            getNavigator().clear();
        }
        return true;
    }
    return TameableEntity::processInteract(player, stack);
}

std::unique_ptr<AgeableEntity> WolfEntity::createChild(AnimalEntity&)
{
    auto child = std::make_unique<WolfEntity>(*world_);
    if (isTamed())
    {
        child->setTamed(true);
        child->setOwnerId(ownerId_);
    }
    return child;
}

core::ResourceLocation WolfEntity::getType() const
{
    return core::ResourceLocation("minecraft:wolf");
}
core::ResourceLocation WolfEntity::getLootTable() const
{
    return core::ResourceLocation("minecraft:entities/wolf");
}
gameplay::MobModelKind WolfEntity::getModelKind() const
{
    return gameplay::MobModelKind::Wolf;
}
core::ResourceLocation WolfEntity::getTexture() const
{
    return core::ResourceLocation("minecraft:entity/wolf/wolf");
}

OcelotEntity::OcelotEntity(World& world) : TameableEntity(world)
{
    setSize(0.6f, 0.7f);
}

void OcelotEntity::initEntityAI()
{
    tasks_.add(1, std::make_unique<ai::SwimGoal>(*this));
    tasks_.add(2, std::make_unique<ai::SitGoal>(*this));
    tasks_.add(3, std::make_unique<ai::TemptGoal>(
        *this, 0.6, std::vector<ItemType>{ItemType::RawFish}, true));
    tasks_.add(5, std::make_unique<ai::FollowOwnerGoal>(*this, 1.0, 10.0f, 5.0f));
    tasks_.add(7, std::make_unique<ai::LeapAtTargetGoal>(*this, 0.3f));
    tasks_.add(8, std::make_unique<ai::AttackMeleeGoal>(*this, 0.8, false));
    tasks_.add(9, std::make_unique<ai::MateGoal>(*this, 0.8));
    tasks_.add(10, std::make_unique<ai::WanderAvoidWaterGoal>(*this, 0.8, 0.00001f));
    tasks_.add(11, std::make_unique<ai::WatchClosestGoal>(*this, 10.0f));
}

void OcelotEntity::applyEntityAttributes()
{
    TameableEntity::applyEntityAttributes();
    getEntityAttribute(SharedMonsterAttributes::MAX_HEALTH).setBaseValue(10.0);
    getEntityAttribute(SharedMonsterAttributes::MOVEMENT_SPEED).setBaseValue(0.30000001192092896);
    attributes_.registerAttribute(SharedMonsterAttributes::ATTACK_DAMAGE).setBaseValue(3.0);
}

bool OcelotEntity::isBreedingItem(ItemType item) const
{
    return item == ItemType::RawFish;
}

bool OcelotEntity::getCanSpawnHere()
{
    const int y = floorInt(posY);
    return y >= 63 && AnimalEntity::getCanSpawnHere();
}

void OcelotEntity::onInitialSpawn()
{
    TameableEntity::onInitialSpawn();
    variant_ = 0;
}

bool OcelotEntity::processInteract(PlayerEntity& player, ItemStack& stack)
{
    if (!isTamed() && stack.item == ItemType::RawFish)
    {
        if (getDistanceSq(player) >= 9.0)
            return false;
        if (stack.count > 1) --stack.count;
        else stack.clear();
        if (rand_.nextInt(3) == 0)
        {
            setTamed(true);
            setOwnerId(player.uuid());
            variant_ = 1 + rand_.nextInt(3);
            setSitting(true);
        }
        return true;
    }
    if (isTamed() && isOwner(player) && !isBreedingItem(stack.item))
    {
        setSitting(!isSitting());
        return true;
    }
    return AnimalEntity::processInteract(player, stack);
}

std::unique_ptr<AgeableEntity> OcelotEntity::createChild(AnimalEntity&)
{
    auto child = std::make_unique<OcelotEntity>(*world_);
    child->variant_ = variant_;
    if (isTamed())
    {
        child->setTamed(true);
        child->setOwnerId(ownerId_);
    }
    return child;
}

core::ResourceLocation OcelotEntity::getType() const
{
    return core::ResourceLocation("minecraft:ocelot");
}
core::ResourceLocation OcelotEntity::getLootTable() const
{
    return core::ResourceLocation("minecraft:entities/ocelot");
}
gameplay::MobModelKind OcelotEntity::getModelKind() const
{
    return gameplay::MobModelKind::Ocelot;
}
core::ResourceLocation OcelotEntity::getTexture() const
{
    return core::ResourceLocation("minecraft:entity/cat/ocelot");
}

ParrotEntity::ParrotEntity(World& world) : TameableEntity(world)
{
    setSize(0.5f, 0.9f);
}

void ParrotEntity::initEntityAI()
{
    tasks_.add(0, std::make_unique<ai::PanicGoal>(*this, 1.25));
    tasks_.add(0, std::make_unique<ai::SwimGoal>(*this));
    tasks_.add(1, std::make_unique<ai::WatchClosestGoal>(*this, 8.0f));
    tasks_.add(2, std::make_unique<ai::SitGoal>(*this));
    tasks_.add(2, std::make_unique<ai::FollowOwnerGoal>(*this, 1.0, 5.0f, 1.0f));
    tasks_.add(2, std::make_unique<ai::WanderAvoidWaterGoal>(*this, 1.0));
}

void ParrotEntity::applyEntityAttributes()
{
    TameableEntity::applyEntityAttributes();
    getEntityAttribute(SharedMonsterAttributes::MAX_HEALTH).setBaseValue(6.0);
    getEntityAttribute(SharedMonsterAttributes::FLYING_SPEED).setBaseValue(0.4000000059604645);
    getEntityAttribute(SharedMonsterAttributes::MOVEMENT_SPEED).setBaseValue(0.20000000298023224);
}

void ParrotEntity::onInitialSpawn()
{
    TameableEntity::onInitialSpawn();
    variant_ = rand_.nextInt(5);
}

bool ParrotEntity::processInteract(PlayerEntity& player, ItemStack& stack)
{
    if (!isTamed() && (stack.item == ItemType::Seeds ||
                       stack.item == ItemType::MelonSeeds ||
                       stack.item == ItemType::PumpkinSeeds ||
                       stack.item == ItemType::BeetrootSeeds))
    {
        if (stack.count > 1) --stack.count;
        else stack.clear();
        if (rand_.nextInt(10) == 0)
        {
            setTamed(true);
            setOwnerId(player.uuid());
        }
        return true;
    }
    if (stack.item == ItemType::Cookie)
    {
        if (stack.count > 1) --stack.count;
        else stack.clear();
        attackEntityFrom(DamageSource::GENERIC, 1000.0f);
        return true;
    }
    if (isTamed() && isOwner(player))
    {
        setSitting(!isSitting());
        return true;
    }
    return false;
}

std::unique_ptr<AgeableEntity> ParrotEntity::createChild(AnimalEntity&)
{
    return std::make_unique<ParrotEntity>(*world_);
}

core::ResourceLocation ParrotEntity::getType() const
{
    return core::ResourceLocation("minecraft:parrot");
}
core::ResourceLocation ParrotEntity::getLootTable() const
{
    return core::ResourceLocation("minecraft:entities/parrot");
}
gameplay::MobModelKind ParrotEntity::getModelKind() const
{
    return gameplay::MobModelKind::Parrot;
}
core::ResourceLocation ParrotEntity::getTexture() const
{
    static const char* names[] = {
        "parrot_red_blue", "parrot_blue", "parrot_green",
        "parrot_yellow_blue", "parrot_grey"};
    return core::ResourceLocation(
        "minecraft",
        std::string("entity/parrot/") + names[std::clamp(variant_, 0, 4)]);
}

AbstractHorseEntity::AbstractHorseEntity(World& world) : AnimalEntity(world)
{
    setSize(1.3964844f, 1.6f);
    stepHeight = 1.0f;
}

void AbstractHorseEntity::initEntityAI()
{
    tasks_.add(0, std::make_unique<ai::SwimGoal>(*this));
    tasks_.add(1, std::make_unique<ai::PanicGoal>(*this, 1.2));
    tasks_.add(2, std::make_unique<ai::MateGoal>(*this, 1.0));
    tasks_.add(4, std::make_unique<ai::FollowParentGoal>(*this, 1.0));
    tasks_.add(6, std::make_unique<ai::WanderAvoidWaterGoal>(*this, 0.7));
    tasks_.add(7, std::make_unique<ai::WatchClosestGoal>(*this, 6.0f));
    tasks_.add(8, std::make_unique<ai::LookIdleGoal>(*this));
}

void AbstractHorseEntity::applyEntityAttributes()
{
    AnimalEntity::applyEntityAttributes();
    getEntityAttribute(SharedMonsterAttributes::MAX_HEALTH).setBaseValue(15.0);
    getEntityAttribute(SharedMonsterAttributes::MOVEMENT_SPEED).setBaseValue(0.22499999403953552);
}

bool AbstractHorseEntity::isBreedingItem(ItemType item) const
{
    return item == ItemType::GoldenCarrot || item == ItemType::GoldenApple;
}

void AbstractHorseEntity::onLivingUpdate()
{
    AnimalEntity::onLivingUpdate();
    if (isBeingRidden() && !tamed_ && rand_.nextInt(50) == 0)
    {
        if (getMaxTemper() > 0 && rand_.nextInt(getMaxTemper()) < temper_)
        {
            tamed_ = true;
            if (auto* rider = dynamic_cast<PlayerEntity*>(getControllingPassenger()))
                loveCause_ = rider->uuid();
        }
        else
        {
            temper_ = std::min(getMaxTemper(), temper_ + 5);
            if (getControllingPassenger())
                getControllingPassenger()->dismountRidingEntity();
        }
    }
}

bool AbstractHorseEntity::processInteract(PlayerEntity& player, ItemStack& stack)
{
    if (AnimalEntity::processInteract(player, stack))
        return true;
    if (tamed_ && !isChild() && stack.item == ItemType::Saddle && !saddled_)
    {
        saddled_ = true;
        if (stack.count > 1) --stack.count;
        else stack.clear();
        return true;
    }
    if (stack.empty() && !isChild())
    {
        player.startRiding(*this);
        return true;
    }
    return false;
}

HorseEntity::HorseEntity(World& world) : AbstractHorseEntity(world) {}

void HorseEntity::applyEntityAttributes()
{
    AbstractHorseEntity::applyEntityAttributes();
    getEntityAttribute(SharedMonsterAttributes::MAX_HEALTH).setBaseValue(22.5);
}

void HorseEntity::onInitialSpawn()
{
    AbstractHorseEntity::onInitialSpawn();
    variant_ = rand_.nextInt(7) + rand_.nextInt(5) * 7;
    if (rand_.nextInt(5) == 0)
        setGrowingAge(-24000);
}

std::unique_ptr<AgeableEntity> HorseEntity::createChild(AnimalEntity& mate)
{
    if (mate.getType().path() == "donkey")
        return std::make_unique<MuleEntity>(*world_);
    return std::make_unique<HorseEntity>(*world_);
}

core::ResourceLocation HorseEntity::getType() const
{
    return core::ResourceLocation("minecraft:horse");
}
core::ResourceLocation HorseEntity::getLootTable() const
{
    return core::ResourceLocation("minecraft:entities/horse");
}
core::ResourceLocation HorseEntity::getTexture() const
{
    static const char* names[] = {
        "horse_white", "horse_creamy", "horse_chestnut", "horse_brown",
        "horse_black", "horse_gray", "horse_darkbrown"};
    return core::ResourceLocation(
        "minecraft",
        std::string("entity/horse/") + names[std::abs(variant_) % 7]);
}

DonkeyEntity::DonkeyEntity(World& world) : AbstractHorseEntity(world) {}
std::unique_ptr<AgeableEntity> DonkeyEntity::createChild(AnimalEntity& mate)
{
    if (mate.getType().path() == "horse")
        return std::make_unique<MuleEntity>(*world_);
    return std::make_unique<DonkeyEntity>(*world_);
}
core::ResourceLocation DonkeyEntity::getType() const
{
    return core::ResourceLocation("minecraft:donkey");
}
core::ResourceLocation DonkeyEntity::getLootTable() const
{
    return core::ResourceLocation("minecraft:entities/donkey");
}
core::ResourceLocation DonkeyEntity::getTexture() const
{
    return core::ResourceLocation("minecraft:entity/horse/donkey");
}

MuleEntity::MuleEntity(World& world) : AbstractHorseEntity(world) {}
std::unique_ptr<AgeableEntity> MuleEntity::createChild(AnimalEntity&)
{
    return nullptr;
}
core::ResourceLocation MuleEntity::getType() const
{
    return core::ResourceLocation("minecraft:mule");
}
core::ResourceLocation MuleEntity::getLootTable() const
{
    return core::ResourceLocation("minecraft:entities/mule");
}
core::ResourceLocation MuleEntity::getTexture() const
{
    return core::ResourceLocation("minecraft:entity/horse/mule");
}

SkeletonHorseEntity::SkeletonHorseEntity(World& world) : AbstractHorseEntity(world)
{
    undead_ = true;
}
std::unique_ptr<AgeableEntity> SkeletonHorseEntity::createChild(AnimalEntity&)
{
    return nullptr;
}
core::ResourceLocation SkeletonHorseEntity::getType() const
{
    return core::ResourceLocation("minecraft:skeleton_horse");
}
core::ResourceLocation SkeletonHorseEntity::getLootTable() const
{
    return core::ResourceLocation("minecraft:entities/skeleton_horse");
}
core::ResourceLocation SkeletonHorseEntity::getTexture() const
{
    return core::ResourceLocation("minecraft:entity/horse/horse_skeleton");
}

ZombieHorseEntity::ZombieHorseEntity(World& world) : AbstractHorseEntity(world)
{
    undead_ = true;
}
std::unique_ptr<AgeableEntity> ZombieHorseEntity::createChild(AnimalEntity&)
{
    return nullptr;
}
core::ResourceLocation ZombieHorseEntity::getType() const
{
    return core::ResourceLocation("minecraft:zombie_horse");
}
core::ResourceLocation ZombieHorseEntity::getLootTable() const
{
    return core::ResourceLocation("minecraft:entities/zombie_horse");
}
core::ResourceLocation ZombieHorseEntity::getTexture() const
{
    return core::ResourceLocation("minecraft:entity/horse/horse_zombie");
}

LlamaEntity::LlamaEntity(World& world) : AbstractHorseEntity(world)
{
    setSize(0.9f, 1.87f);
}

void LlamaEntity::initEntityAI()
{
    AbstractHorseEntity::initEntityAI();
    tasks_.add(3, std::make_unique<ai::AttackRangedBowGoal>(*this, 1.25, 40, 20.0f));
}

void LlamaEntity::applyEntityAttributes()
{
    AbstractHorseEntity::applyEntityAttributes();
    getEntityAttribute(SharedMonsterAttributes::MAX_HEALTH).setBaseValue(22.0);
    getEntityAttribute(SharedMonsterAttributes::FOLLOW_RANGE).setBaseValue(40.0);
    attributes_.registerAttribute(SharedMonsterAttributes::ATTACK_DAMAGE).setBaseValue(1.0);
}

void LlamaEntity::onInitialSpawn()
{
    AbstractHorseEntity::onInitialSpawn();
    variant_ = rand_.nextInt(4);
}

bool LlamaEntity::attackEntityAsMob(Entity& target)
{
    auto spit = std::make_unique<LlamaSpitEntity>(getWorld(), this);
    const double dx = target.posX - posX;
    const double dy = target.posY + target.getHeight() * 0.333 - spit->posY;
    const double dz = target.posZ - posZ;
    const float dist = static_cast<float>(std::sqrt(dx * dx + dz * dz));
    spit->shoot(dx, dy + dist * 0.2, dz, 1.5f, 10.0f);
    getWorld().spawnEntity(std::move(spit));
    return true;
}

std::unique_ptr<AgeableEntity> LlamaEntity::createChild(AnimalEntity& mate)
{
    auto child = std::make_unique<LlamaEntity>(*world_);
    child->variant_ = rand_.nextBoolean()
        ? variant_
        : static_cast<LlamaEntity&>(mate).variant_;
    return child;
}

core::ResourceLocation LlamaEntity::getType() const
{
    return core::ResourceLocation("minecraft:llama");
}
core::ResourceLocation LlamaEntity::getLootTable() const
{
    return core::ResourceLocation("minecraft:entities/llama");
}
gameplay::MobModelKind LlamaEntity::getModelKind() const
{
    return gameplay::MobModelKind::Llama;
}
core::ResourceLocation LlamaEntity::getTexture() const
{
    static const char* names[] = {
        "llama_creamy", "llama_white", "llama_brown", "llama_gray"};
    return core::ResourceLocation(
        "minecraft",
        std::string("entity/llama/") + names[std::clamp(variant_, 0, 3)]);
}
}
