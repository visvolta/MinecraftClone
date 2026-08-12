#include "entity/ai/VanillaGoals.h"

#include "World.h"
#include "entity/AnimalEntity.h"
#include "entity/Creature.h"
#include "entity/Math.h"
#include "entity/Mob.h"
#include "entity/PlayerEntity.h"
#include "entity/TameableEntity.h"
#include "entity/mobs/CreeperEntity.h"

#include <cmath>
#include <limits>

namespace mc::entity::ai
{
namespace
{
PlayerEntity* closestPlayer(Mob& mob, float range)
{
    return mob.getWorld().getClosestPlayer(
        mob.posX, mob.posY, mob.posZ, range);
}

bool hasItem(const std::vector<ItemType>& items, ItemType item)
{
    return std::find(items.begin(), items.end(), item) != items.end();
}
}

std::optional<glm::dvec3> findRandomTarget(
    Mob& mob,
    int xz,
    int y,
    const glm::dvec3* direction,
    bool avoidWater)
{
    float best = -99999.0f;
    std::optional<glm::dvec3> result;
    World& world = mob.getWorld();
    for (int i = 0; i < 10; ++i)
    {
        const int ox = mob.getRNG().nextInt(2 * xz + 1) - xz;
        const int oy = mob.getRNG().nextInt(2 * y + 1) - y;
        const int oz = mob.getRNG().nextInt(2 * xz + 1) - xz;
        if (direction &&
            static_cast<double>(ox) * direction->x +
                static_cast<double>(oz) * direction->z < 0.0)
            continue;
        int cx = floorInt(mob.posX) + ox;
        int cy = floorInt(mob.posY) + oy;
        int cz = floorInt(mob.posZ) + oz;
        if (!world.isBlockLoaded(cx, cy, cz))
            continue;
        while (cy < 254 && world.isSolidBlock(cx, cy, cz))
            ++cy;
        if (avoidWater && world.getBlock(cx, cy, cz) == BlockType::Water)
            continue;
        if (world.isSolidBlock(cx, cy, cz) ||
            world.isSolidBlock(cx, cy + 1, cz) ||
            !world.isSolidBlock(cx, cy - 1, cz))
            continue;
        float weight = world.getLightBrightness(cx, cy, cz) - 0.5f;
        if (world.getBlock(cx, cy - 1, cz) == BlockType::Grass)
            weight = 10.0f;
        if (weight > best)
        {
            best = weight;
            result = glm::dvec3(cx + 0.5, cy, cz + 0.5);
        }
    }
    return result;
}

SwimGoal::SwimGoal(Mob& mob) : mob_(&mob)
{
    setMutexBits(4);
}

bool SwimGoal::shouldExecute(GoalContext&)
{
    return mob_->isInWater() || mob_->isInLava();
}

void SwimGoal::tick(GoalContext&)
{
    if (mob_->getRNG().nextFloat() < 0.8f)
        mob_->getJumpHelper().setJumping();
}

PanicGoal::PanicGoal(Mob& mob, double speed) : mob_(&mob), speed_(speed)
{
    setMutexBits(1);
}

bool PanicGoal::shouldExecute(GoalContext&)
{
    if (mob_->getRevengeTarget() == nullptr && !mob_->isBurning())
        return false;
    if (const auto target = findRandomTarget(*mob_, 5, 4))
    {
        targetX_ = target->x;
        targetY_ = target->y;
        targetZ_ = target->z;
        return true;
    }
    return false;
}

bool PanicGoal::shouldContinue(GoalContext&)
{
    return !mob_->getNavigator().noPath();
}

void PanicGoal::start(GoalContext&)
{
    navigation::WorldNavigationBlockAccess access(mob_->getWorld());
    mob_->getNavigator().tryMoveTo(
        access, mob_->getPositionVec(),
        {static_cast<float>(targetX_), static_cast<float>(targetY_),
         static_cast<float>(targetZ_)},
        speed_,
        16.0f);
}

WanderAvoidWaterGoal::WanderAvoidWaterGoal(Mob& mob, double speed, float chance)
    : mob_(&mob), speed_(speed), chance_(chance)
{
    setMutexBits(1);
}

bool WanderAvoidWaterGoal::shouldExecute(GoalContext&)
{
    if (mob_->getRNG().nextFloat() >= chance_)
        return false;
    if (const auto target = findRandomTarget(*mob_, 10, 7, nullptr, true))
    {
        targetX_ = target->x;
        targetY_ = target->y;
        targetZ_ = target->z;
        return true;
    }
    return false;
}

bool WanderAvoidWaterGoal::shouldContinue(GoalContext&)
{
    return !mob_->getNavigator().noPath();
}

void WanderAvoidWaterGoal::start(GoalContext&)
{
    navigation::WorldNavigationBlockAccess access(mob_->getWorld());
    mob_->getNavigator().tryMoveTo(
        access, mob_->getPositionVec(),
        {static_cast<float>(targetX_), static_cast<float>(targetY_),
         static_cast<float>(targetZ_)},
        speed_, 16.0f);
}

WatchClosestGoal::WatchClosestGoal(Mob& mob, float range, float chance)
    : mob_(&mob), range_(range), chance_(chance)
{
    setMutexBits(2);
}

bool WatchClosestGoal::shouldExecute(GoalContext&)
{
    if (mob_->getRNG().nextFloat() >= chance_)
        return false;
    closest_ = closestPlayer(*mob_, range_);
    return closest_ != nullptr && closest_->isAlive();
}

bool WatchClosestGoal::shouldContinue(GoalContext&)
{
    return closest_ && closest_->isAlive() && lookTime_ > 0 &&
           mob_->getDistanceSq(*closest_) <= static_cast<double>(range_ * range_);
}

void WatchClosestGoal::start(GoalContext&)
{
    lookTime_ = 40 + mob_->getRNG().nextInt(40);
}

void WatchClosestGoal::tick(GoalContext&)
{
    if (closest_)
        mob_->getLookHelper().setLookPositionWithEntity(*closest_, 10.0f, 40.0f);
    --lookTime_;
}

LookIdleGoal::LookIdleGoal(Mob& mob) : mob_(&mob)
{
    setMutexBits(3);
}

bool LookIdleGoal::shouldExecute(GoalContext&)
{
    return mob_->getRNG().nextFloat() < 0.02f;
}

bool LookIdleGoal::shouldContinue(GoalContext&)
{
    return idleTime_ >= 0;
}

void LookIdleGoal::start(GoalContext&)
{
    const double angle = 6.283185307179586 * mob_->getRNG().nextDouble();
    lookX_ = std::cos(angle);
    lookZ_ = std::sin(angle);
    idleTime_ = 20 + mob_->getRNG().nextInt(20);
}

void LookIdleGoal::tick(GoalContext&)
{
    --idleTime_;
    mob_->getLookHelper().setLookPosition(
        mob_->posX + lookX_,
        mob_->posY + mob_->getEyeHeight(),
        mob_->posZ + lookZ_,
        10.0f, 40.0f);
}

AttackMeleeGoal::AttackMeleeGoal(Mob& mob, double speed, bool longMemory)
    : mob_(&mob), speed_(speed), longMemory_(longMemory)
{
    setMutexBits(3);
}

bool AttackMeleeGoal::shouldExecute(GoalContext&)
{
    LivingEntity* target = mob_->getAttackTarget();
    return target && target->isAlive();
}

bool AttackMeleeGoal::shouldContinue(GoalContext&)
{
    LivingEntity* target = mob_->getAttackTarget();
    if (!target || !target->isAlive())
        return false;
    return longMemory_ || mob_->getEntitySenses().canSee(*target);
}

void AttackMeleeGoal::start(GoalContext&)
{
    delayCounter_ = 0;
}

void AttackMeleeGoal::reset(GoalContext&)
{
    mob_->getNavigator().clear();
}

void AttackMeleeGoal::tick(GoalContext&)
{
    LivingEntity* target = mob_->getAttackTarget();
    if (!target)
        return;
    mob_->getLookHelper().setLookPositionWithEntity(*target, 30.0f, 30.0f);
    const double dist = mob_->getDistanceSq(*target);
    if (--delayCounter_ <= 0)
    {
        delayCounter_ = 4 + mob_->getRNG().nextInt(7);
        if (dist > 1024.0)
            delayCounter_ += 10;
        else if (dist > 256.0)
            delayCounter_ += 5;
        navigation::WorldNavigationBlockAccess access(mob_->getWorld());
        if (!mob_->getNavigator().tryMoveTo(
                access, mob_->getPositionVec(), target->getPositionVec(),
                speed_, 16.0f))
            delayCounter_ += 15;
    }
    const double reach = mob_->getWidth() * 2.0f * mob_->getWidth() * 2.0f;
    if (dist <= reach + 0.6 && mob_->hurtResistantTime <= 1)
        mob_->attackEntityAsMob(*target);
}

NearestAttackableTargetGoal::NearestAttackableTargetGoal(
    Mob& mob, bool playersOnly, float range, bool checkSight)
    : mob_(&mob), playersOnly_(playersOnly), range_(range), checkSight_(checkSight)
{
}

bool NearestAttackableTargetGoal::shouldExecute(GoalContext&)
{
    float range = range_;
    if (range <= 0.0f)
        range = static_cast<float>(
            mob_->getEntityAttribute(SharedMonsterAttributes::FOLLOW_RANGE)
                .getAttributeValue());
    if (playersOnly_)
    {
        target_ = closestPlayer(*mob_, range);
        if (!target_ || !target_->isAlive())
            return false;
        if (checkSight_ && !mob_->getEntitySenses().canSee(*target_))
            return false;
        return true;
    }
    LivingEntity* best = nullptr;
    double bestDist = range * range;
    for (Mob* other : mob_->getWorld().getMobs())
    {
        if (other == mob_ || !other->isAlive())
            continue;
        const double d = mob_->getDistanceSq(*other);
        if (d < bestDist)
        {
            bestDist = d;
            best = other;
        }
    }
    target_ = best;
    return target_ != nullptr;
}

bool NearestAttackableTargetGoal::shouldContinue(GoalContext&)
{
    return mob_->getAttackTarget() && mob_->getAttackTarget()->isAlive();
}

void NearestAttackableTargetGoal::start(GoalContext&)
{
    mob_->setAttackTarget(target_);
}

HurtByTargetGoal::HurtByTargetGoal(Mob& mob, bool callForHelp)
    : mob_(&mob), callForHelp_(callForHelp)
{
}

bool HurtByTargetGoal::shouldExecute(GoalContext&)
{
    LivingEntity* revenge = mob_->getRevengeTarget();
    return revenge && revenge->isAlive();
}

void HurtByTargetGoal::start(GoalContext&)
{
    mob_->setAttackTarget(mob_->getRevengeTarget());
    if (callForHelp_)
    {
        for (Mob* other : mob_->getWorld().getMobs())
        {
            if (other == mob_ || other->getType() != mob_->getType())
                continue;
            if (other->getDistanceSq(*mob_) < 100.0 &&
                other->getAttackTarget() == nullptr)
                other->setAttackTarget(mob_->getRevengeTarget());
        }
    }
}

MateGoal::MateGoal(AnimalEntity& animal, double speed)
    : animal_(&animal), speed_(speed)
{
    setMutexBits(3);
}

bool MateGoal::shouldExecute(GoalContext&)
{
    if (!animal_->isInLove())
        return false;
    mate_ = nullptr;
    double best = 64.0;
    for (Mob* other : animal_->getWorld().getMobs())
    {
        auto* animal = dynamic_cast<AnimalEntity*>(other);
        if (!animal || animal == animal_ || !animal_->canMateWith(*animal))
            continue;
        const double d = animal_->getDistanceSq(*animal);
        if (d < best)
        {
            best = d;
            mate_ = animal;
        }
    }
    return mate_ != nullptr;
}

bool MateGoal::shouldContinue(GoalContext&)
{
    return mate_ && mate_->isAlive() && mate_->isInLove() && spawnBabyDelay_ < 60;
}

void MateGoal::start(GoalContext&)
{
    spawnBabyDelay_ = 0;
}

void MateGoal::tick(GoalContext&)
{
    animal_->getLookHelper().setLookPositionWithEntity(*mate_, 10.0f, 40.0f);
    navigation::WorldNavigationBlockAccess access(animal_->getWorld());
    animal_->getNavigator().tryMoveTo(
        access, animal_->getPositionVec(), mate_->getPositionVec(), speed_, 16.0f);
    ++spawnBabyDelay_;
    if (spawnBabyDelay_ >= 60 && animal_->getDistanceSq(*mate_) < 9.0)
        animal_->spawnChildFromBreeding(*mate_);
}

TemptGoal::TemptGoal(
    Creature& creature,
    double speed,
    std::vector<ItemType> items,
    bool scaredByPlayerMovement)
    : creature_(&creature),
      speed_(speed),
      items_(std::move(items)),
      scared_(scaredByPlayerMovement)
{
    setMutexBits(3);
}

bool TemptGoal::shouldExecute(GoalContext&)
{
    if (delayTemptCounter_ > 0)
    {
        --delayTemptCounter_;
        return false;
    }
    temptingPlayer_ = closestPlayer(*creature_, 10.0f);
    if (!temptingPlayer_)
        return false;
    return hasItem(items_, temptingPlayer_->getHeldItemType());
}

bool TemptGoal::shouldContinue(GoalContext&)
{
    return temptingPlayer_ && temptingPlayer_->isAlive() &&
           hasItem(items_, temptingPlayer_->getHeldItemType()) &&
           creature_->getDistanceSq(*temptingPlayer_) < 100.0;
}

void TemptGoal::start(GoalContext&) {}

void TemptGoal::reset(GoalContext&)
{
    temptingPlayer_ = nullptr;
    creature_->getNavigator().clear();
    delayTemptCounter_ = 100;
}

void TemptGoal::tick(GoalContext&)
{
    creature_->getLookHelper().setLookPositionWithEntity(
        *temptingPlayer_, 30.0f, 40.0f);
    if (creature_->getDistanceSq(*temptingPlayer_) < 6.25)
        creature_->getNavigator().clear();
    else
    {
        navigation::WorldNavigationBlockAccess access(creature_->getWorld());
        creature_->getNavigator().tryMoveTo(
            access, creature_->getPositionVec(),
            temptingPlayer_->getPositionVec(), speed_, 16.0f);
    }
}

FollowParentGoal::FollowParentGoal(AnimalEntity& animal, double speed)
    : animal_(&animal), speed_(speed)
{
}

bool FollowParentGoal::shouldExecute(GoalContext&)
{
    if (!animal_->isChild())
        return false;
    parent_ = nullptr;
    double best = 64.0;
    for (Mob* other : animal_->getWorld().getMobs())
    {
        auto* adult = dynamic_cast<AnimalEntity*>(other);
        if (!adult || adult->isChild() || adult->getType() != animal_->getType())
            continue;
        const double d = animal_->getDistanceSq(*adult);
        if (d >= 9.0 && d < best)
        {
            best = d;
            parent_ = adult;
        }
    }
    return parent_ != nullptr;
}

bool FollowParentGoal::shouldContinue(GoalContext&)
{
    if (!parent_ || parent_->isDead())
        return false;
    const double d = animal_->getDistanceSq(*parent_);
    return d >= 9.0 && d <= 256.0;
}

void FollowParentGoal::tick(GoalContext&)
{
    if (--delay_ <= 0)
    {
        delay_ = 10;
        navigation::WorldNavigationBlockAccess access(animal_->getWorld());
        animal_->getNavigator().tryMoveTo(
            access, animal_->getPositionVec(), parent_->getPositionVec(),
            speed_, 16.0f);
    }
}

EatGrassGoal::EatGrassGoal(Mob& mob) : mob_(&mob)
{
    setMutexBits(7);
}

bool EatGrassGoal::shouldExecute(GoalContext&)
{
    if (mob_->getRNG().nextInt(mob_->isChild() ? 50 : 1000) != 0)
        return false;
    const int x = floorInt(mob_->posX);
    const int y = floorInt(mob_->posY);
    const int z = floorInt(mob_->posZ);
    return mob_->getWorld().getBlock(x, y, z) == BlockType::TallGrass ||
           mob_->getWorld().getBlock(x, y - 1, z) == BlockType::Grass;
}

bool EatGrassGoal::shouldContinue(GoalContext&)
{
    return timer_ > 0;
}

void EatGrassGoal::start(GoalContext&)
{
    timer_ = 40;
    mob_->getNavigator().clear();
}

void EatGrassGoal::reset(GoalContext&)
{
    timer_ = 0;
}

void EatGrassGoal::tick(GoalContext&)
{
    timer_ = std::max(0, timer_ - 1);
    if (timer_ != 4)
        return;
    const int x = floorInt(mob_->posX);
    const int y = floorInt(mob_->posY);
    const int z = floorInt(mob_->posZ);
    World& world = mob_->getWorld();
    if (world.getBlock(x, y, z) == BlockType::TallGrass)
        world.setBlock(x, y, z, BlockType::Air);
    else if (world.getBlock(x, y - 1, z) == BlockType::Grass)
        world.setBlock(x, y - 1, z, BlockType::Dirt);
    mob_->eatGrassBonus();
}

SitGoal::SitGoal(TameableEntity& tameable) : tameable_(&tameable)
{
    setMutexBits(5);
}

bool SitGoal::shouldExecute(GoalContext&)
{
    if (!tameable_->isTamed() || tameable_->isInWater())
        return false;
    return tameable_->isSitting();
}

void SitGoal::start(GoalContext&)
{
    tameable_->getNavigator().clear();
    tameable_->setAttackTarget(nullptr);
}

void SitGoal::reset(GoalContext&)
{
    tameable_->setSitting(false);
}

FollowOwnerGoal::FollowOwnerGoal(
    TameableEntity& tameable, double speed, float minDist, float maxDist)
    : tameable_(&tameable), speed_(speed), minDist_(minDist), maxDist_(maxDist)
{
    setMutexBits(3);
}

bool FollowOwnerGoal::shouldExecute(GoalContext&)
{
    PlayerEntity* owner = tameable_->getOwner();
    return owner && tameable_->isTamed() && !tameable_->isSitting() &&
           tameable_->getDistanceSq(*owner) >= minDist_ * minDist_;
}

bool FollowOwnerGoal::shouldContinue(GoalContext&)
{
    PlayerEntity* owner = tameable_->getOwner();
    return owner && !tameable_->getNavigator().noPath() &&
           !tameable_->isSitting() &&
           tameable_->getDistanceSq(*owner) > maxDist_ * maxDist_;
}

void FollowOwnerGoal::start(GoalContext&)
{
    timeToRecalc_ = 0;
}

void FollowOwnerGoal::reset(GoalContext&)
{
    tameable_->getNavigator().clear();
}

void FollowOwnerGoal::tick(GoalContext&)
{
    PlayerEntity* owner = tameable_->getOwner();
    if (!owner)
        return;
    tameable_->getLookHelper().setLookPositionWithEntity(*owner, 10.0f, 40.0f);
    if (--timeToRecalc_ <= 0)
    {
        timeToRecalc_ = 10;
        navigation::WorldNavigationBlockAccess access(tameable_->getWorld());
        if (!tameable_->getNavigator().tryMoveTo(
                access, tameable_->getPositionVec(), owner->getPositionVec(),
                speed_, 16.0f) &&
            tameable_->getDistanceSq(*owner) >= 144.0)
        {
            tameable_->setPosition(owner->posX, owner->posY, owner->posZ);
        }
    }
}

LeapAtTargetGoal::LeapAtTargetGoal(Mob& mob, float leapMotionY)
    : mob_(&mob), leapMotionY_(leapMotionY)
{
    setMutexBits(5);
}

bool LeapAtTargetGoal::shouldExecute(GoalContext&)
{
    LivingEntity* target = mob_->getAttackTarget();
    if (!target || !mob_->onGround)
        return false;
    const double d = mob_->getDistanceSq(*target);
    return d >= 4.0 && d <= 16.0 && mob_->getRNG().nextInt(5) == 0;
}

bool LeapAtTargetGoal::shouldContinue(GoalContext&)
{
    return !mob_->onGround;
}

void LeapAtTargetGoal::start(GoalContext&)
{
    LivingEntity* target = mob_->getAttackTarget();
    if (!target)
        return;
    double dx = target->posX - mob_->posX;
    double dz = target->posZ - mob_->posZ;
    const float len = static_cast<float>(std::sqrt(dx * dx + dz * dz));
    if (len >= 1.0e-4f)
    {
        mob_->motionX += dx / len * 0.5 * 0.800000011920929 +
            mob_->motionX * 0.20000000298023224;
        mob_->motionZ += dz / len * 0.5 * 0.800000011920929 +
            mob_->motionZ * 0.20000000298023224;
    }
    mob_->motionY = leapMotionY_;
}

AvoidEntityGoal::AvoidEntityGoal(
    Creature& creature,
    float avoidDistance,
    double farSpeed,
    double nearSpeed,
    std::function<bool(LivingEntity&)> selector)
    : creature_(&creature),
      avoidDistance_(avoidDistance),
      farSpeed_(farSpeed),
      nearSpeed_(nearSpeed),
      selector_(std::move(selector))
{
    setMutexBits(1);
}

bool AvoidEntityGoal::shouldExecute(GoalContext&)
{
    closest_ = nullptr;
    double best = avoidDistance_ * avoidDistance_;
    if (PlayerEntity* player = closestPlayer(*creature_, avoidDistance_))
    {
        if (selector_(*player))
        {
            closest_ = player;
            best = creature_->getDistanceSq(*player);
        }
    }
    if (!closest_)
        return false;
    glm::dvec3 away{
        creature_->posX - closest_->posX, 0.0,
        creature_->posZ - closest_->posZ};
    if (const auto target = findRandomTarget(*creature_, 16, 7, &away, true))
    {
        if (closest_->getDistanceSq(target->x, target->y, target->z) < best)
            return false;
        targetX_ = target->x;
        targetY_ = target->y;
        targetZ_ = target->z;
        return true;
    }
    return false;
}

bool AvoidEntityGoal::shouldContinue(GoalContext&)
{
    return !creature_->getNavigator().noPath();
}

void AvoidEntityGoal::start(GoalContext&)
{
    navigation::WorldNavigationBlockAccess access(creature_->getWorld());
    creature_->getNavigator().tryMoveTo(
        access, creature_->getPositionVec(),
        {static_cast<float>(targetX_), static_cast<float>(targetY_),
         static_cast<float>(targetZ_)},
        farSpeed_, 16.0f);
}

void AvoidEntityGoal::reset(GoalContext&)
{
    closest_ = nullptr;
}

CreeperSwellGoal::CreeperSwellGoal(Mob& creeper) : creeper_(&creeper)
{
    setMutexBits(1);
}

bool CreeperSwellGoal::shouldExecute(GoalContext&)
{
    LivingEntity* target = creeper_->getAttackTarget();
    return creeper_->getAttackProgress() > 0.0f ||
           (target && creeper_->getDistanceSq(*target) < 9.0);
}

void CreeperSwellGoal::start(GoalContext&)
{
    creeper_->getNavigator().clear();
}

void CreeperSwellGoal::reset(GoalContext&)
{
    if (auto* creeper = dynamic_cast<CreeperEntity*>(creeper_))
        creeper->setCreeperState(-1);
}

void CreeperSwellGoal::tick(GoalContext&)
{
    auto* creeper = dynamic_cast<CreeperEntity*>(creeper_);
    if (!creeper)
        return;
    LivingEntity* target = creeper->getAttackTarget();
    if (!target || creeper->getDistanceSq(*target) > 49.0 ||
        !creeper->getEntitySenses().canSee(*target))
        creeper->setCreeperState(-1);
    else
        creeper->setCreeperState(1);
}

AttackRangedBowGoal::AttackRangedBowGoal(
    Mob& mob, double speed, int attackCooldown, float maxDistance)
    : mob_(&mob),
      speed_(speed),
      attackCooldown_(attackCooldown),
      maxAttackDistance_(maxDistance)
{
    setMutexBits(3);
}

bool AttackRangedBowGoal::shouldExecute(GoalContext&)
{
    return mob_->getAttackTarget() && mob_->getAttackTarget()->isAlive();
}

bool AttackRangedBowGoal::shouldContinue(GoalContext&)
{
    return shouldExecute(*mob_);
}

void AttackRangedBowGoal::start(GoalContext&) {}

void AttackRangedBowGoal::reset(GoalContext&)
{
    seeTime_ = 0;
    attackTime_ = -1;
    mob_->getNavigator().clear();
}

void AttackRangedBowGoal::tick(GoalContext&)
{
    LivingEntity* target = mob_->getAttackTarget();
    if (!target)
        return;
    const double dist = mob_->getDistanceSq(*target);
    const bool seen = mob_->getEntitySenses().canSee(*target);
    if (seen)
        ++seeTime_;
    else
        seeTime_ = 0;
    if (dist <= maxAttackDistance_ * maxAttackDistance_ && seeTime_ >= 20)
        mob_->getNavigator().clear();
    else
    {
        navigation::WorldNavigationBlockAccess access(mob_->getWorld());
        mob_->getNavigator().tryMoveTo(
            access, mob_->getPositionVec(), target->getPositionVec(),
            speed_, 16.0f);
    }
    mob_->getLookHelper().setLookPositionWithEntity(*target, 30.0f, 30.0f);
    if (--attackTime_ <= 0 && seen)
    {
        attackTime_ = attackCooldown_;
        mob_->attackEntityAsMob(*target);
    }
}

BegGoal::BegGoal(TameableEntity& wolf, float minDistance)
    : wolf_(&wolf), minDistance_(minDistance)
{
    setMutexBits(2);
}

bool BegGoal::shouldExecute(GoalContext&)
{
    player_ = closestPlayer(*wolf_, minDistance_);
    if (!player_)
        return false;
    const ItemType held = player_->getHeldItemType();
    return held == ItemType::Bone || wolf_->isBreedingItem(held);
}

bool BegGoal::shouldContinue(GoalContext&)
{
    return player_ && player_->isAlive() && timeout_ > 0 &&
           wolf_->getDistanceSq(*player_) < minDistance_ * minDistance_;
}

void BegGoal::start(GoalContext&)
{
    timeout_ = 40 + wolf_->getRNG().nextInt(40);
}

void BegGoal::reset(GoalContext&)
{
    player_ = nullptr;
}

void BegGoal::tick(GoalContext&)
{
    wolf_->getLookHelper().setLookPositionWithEntity(*player_, 40.0f, 40.0f);
    --timeout_;
}

RestrictSunGoal::RestrictSunGoal(Creature& creature) : creature_(&creature) {}

bool RestrictSunGoal::shouldExecute(GoalContext&)
{
    return creature_->getWorld().isDaytime();
}

void RestrictSunGoal::start(GoalContext&) {}
void RestrictSunGoal::reset(GoalContext&) {}

FleeSunGoal::FleeSunGoal(Creature& creature, double movementSpeed)
    : creature_(&creature), speed_(movementSpeed)
{
    setMutexBits(1);
}

bool FleeSunGoal::shouldExecute(GoalContext&)
{
    if (!creature_->getWorld().isDaytime() || !creature_->isBurning())
        return false;
    World& world = creature_->getWorld();
    int best = 15;
    bool found = false;
    for (int i = 0; i < 10; ++i)
    {
        const int x = floorInt(creature_->posX) + creature_->getRNG().nextInt(20) - 10;
        const int y = floorInt(creature_->posY) + creature_->getRNG().nextInt(6) - 3;
        const int z = floorInt(creature_->posZ) + creature_->getRNG().nextInt(20) - 10;
        const int light = world.getSkyLightLevel(x, y, z);
        if (light < best && world.getBlock(x, y, z) == BlockType::Air)
        {
            best = light;
            shelterX_ = x + 0.5;
            shelterY_ = y;
            shelterZ_ = z + 0.5;
            found = true;
        }
    }
    return found;
}

bool FleeSunGoal::shouldContinue(GoalContext&)
{
    return !creature_->getNavigator().noPath();
}

void FleeSunGoal::start(GoalContext&)
{
    navigation::WorldNavigationBlockAccess access(creature_->getWorld());
    creature_->getNavigator().tryMoveTo(
        access, creature_->getPositionVec(),
        {static_cast<float>(shelterX_), static_cast<float>(shelterY_),
         static_cast<float>(shelterZ_)},
        speed_, 16.0f);
}

MoveTowardsRestrictionGoal::MoveTowardsRestrictionGoal(
    Creature& creature, double speed)
    : creature_(&creature), speed_(speed)
{
    setMutexBits(1);
}

bool MoveTowardsRestrictionGoal::shouldExecute(GoalContext&)
{
    if (!creature_->hasHome() || creature_->isWithinHomeDistanceCurrentPosition())
        return false;
    targetX_ = creature_->posX;
    targetY_ = creature_->posY;
    targetZ_ = creature_->posZ;
    return true;
}

bool MoveTowardsRestrictionGoal::shouldContinue(GoalContext&)
{
    return !creature_->getNavigator().noPath();
}

void MoveTowardsRestrictionGoal::start(GoalContext&)
{
    navigation::WorldNavigationBlockAccess access(creature_->getWorld());
    creature_->getNavigator().tryMoveTo(
        access, creature_->getPositionVec(),
        creature_->getPositionVec(), speed_, 16.0f);
}
}
