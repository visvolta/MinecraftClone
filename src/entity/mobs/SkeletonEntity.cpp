#include "entity/mobs/SkeletonEntity.h"

#include "World.h"
#include "entity/Math.h"
#include "entity/ai/VanillaGoals.h"
#include "entity/projectile/ArrowEntity.h"

#include <cmath>

namespace mc::entity
{
AbstractSkeletonEntity::AbstractSkeletonEntity(World& world)
    : MonsterEntity(world)
{
    setSize(0.6f, 1.99f);
    experienceValue_ = 5;
    undead_ = true;
}

void AbstractSkeletonEntity::initEntityAI()
{
    tasks_.add(1, std::make_unique<ai::SwimGoal>(*this));
    tasks_.add(2, std::make_unique<ai::RestrictSunGoal>(*this));
    tasks_.add(3, std::make_unique<ai::FleeSunGoal>(*this, 1.0));
    tasks_.add(5, std::make_unique<ai::WanderAvoidWaterGoal>(*this, 1.0));
    tasks_.add(6, std::make_unique<ai::WatchClosestGoal>(*this, 8.0f));
    tasks_.add(6, std::make_unique<ai::LookIdleGoal>(*this));
    tasks_.add(4, std::make_unique<ai::AttackRangedBowGoal>(*this, 1.0, 20, 15.0f));
    targetTasks_.add(1, std::make_unique<ai::HurtByTargetGoal>(*this, false));
    targetTasks_.add(2, std::make_unique<ai::NearestAttackableTargetGoal>(*this, true));
}

void AbstractSkeletonEntity::applyEntityAttributes()
{
    MonsterEntity::applyEntityAttributes();
    getEntityAttribute(SharedMonsterAttributes::MOVEMENT_SPEED).setBaseValue(0.25);
}

void AbstractSkeletonEntity::onLivingUpdate()
{
    if (world_->isDaytime() && shouldBurnInDay())
    {
        const float brightness = getBrightness();
        if (brightness > 0.5f &&
            rand_.nextFloat() * 30.0f < (brightness - 0.4f) * 2.0f &&
            world_->canSeeSky(
                floorInt(posX), floorInt(posY + getEyeHeight()), floorInt(posZ)))
            setFire(8);
    }
    MonsterEntity::onLivingUpdate();
}

bool AbstractSkeletonEntity::attackEntityAsMob(Entity& target)
{
    auto arrow = std::make_unique<ArrowEntity>(getWorld(), this);
    const double dx = target.posX - posX;
    const double dz = target.posZ - posZ;
    const double dy = target.getEntityBoundingBox().minY +
        static_cast<double>(target.getHeight()) * 0.333 - arrow->posY;
    const float dist = static_cast<float>(std::sqrt(dx * dx + dz * dz));
    const float inaccuracy = static_cast<float>(
        14 - difficultyId(world_->getDifficulty()) * 4);
    arrow->shoot(dx, dy + dist * 0.20000000298023224, dz, 1.6f, inaccuracy);
    getWorld().spawnEntity(std::move(arrow));
    swingArm();
    return true;
}

SkeletonEntity::SkeletonEntity(World& world) : AbstractSkeletonEntity(world) {}

core::ResourceLocation SkeletonEntity::getType() const
{
    return core::ResourceLocation("minecraft:skeleton");
}
core::ResourceLocation SkeletonEntity::getLootTable() const
{
    return core::ResourceLocation("minecraft:entities/skeleton");
}
gameplay::MobModelKind SkeletonEntity::getModelKind() const
{
    return gameplay::MobModelKind::Skeleton;
}
core::ResourceLocation SkeletonEntity::getTexture() const
{
    return core::ResourceLocation("minecraft:entity/skeleton/skeleton");
}

StrayEntity::StrayEntity(World& world) : AbstractSkeletonEntity(world) {}

core::ResourceLocation StrayEntity::getType() const
{
    return core::ResourceLocation("minecraft:stray");
}
core::ResourceLocation StrayEntity::getLootTable() const
{
    return core::ResourceLocation("minecraft:entities/stray");
}
gameplay::MobModelKind StrayEntity::getModelKind() const
{
    return gameplay::MobModelKind::Skeleton;
}
core::ResourceLocation StrayEntity::getTexture() const
{
    return core::ResourceLocation("minecraft:entity/skeleton/stray");
}
core::ResourceLocation StrayEntity::getOverlayTexture() const
{
    return core::ResourceLocation("minecraft:entity/skeleton/stray_overlay");
}
}
