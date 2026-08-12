#include "entity/Mob.h"

#include "World.h"
#include "entity/Math.h"
#include "entity/PlayerEntity.h"

#include <algorithm>
#include <cmath>

namespace mc::entity
{
Mob::Mob(World& world)
    : LivingEntity(world),
      lookHelper_(*this),
      moveHelper_(*this),
      jumpHelper_(*this),
      bodyHelper_(*this),
      navigator_(createNavigationSettings()),
      senses_(*this)
{
    applyEntityAttributes();
    for (int i = 0; i < static_cast<int>(navigation::PathNodeType::Count); ++i)
        pathPriorities_[i] = navigation::defaultPriority(
            static_cast<navigation::PathNodeType>(i));
    initEntityAI();
}

void Mob::applyEntityAttributes()
{
    LivingEntity::applyEntityAttributes();
    attributes_.registerAttribute(SharedMonsterAttributes::FOLLOW_RANGE)
        .setBaseValue(16.0);
}

navigation::NavigationSettings Mob::createNavigationSettings() const
{
    navigation::NavigationSettings settings;
    settings.width = width_;
    settings.height = height_;
    settings.stepHeight = stepHeight;
    settings.maximumFallHeight = 3;
    settings.canSwim = true;
    return settings;
}

void Mob::onUpdate()
{
    LivingEntity::onUpdate();
    if (isDead())
        return;
    updateLeashedState();
    if (ticksExisted_ % 5 == 0)
    {
        const bool flag = !(getControllingPassenger() &&
                            getControllingPassenger()->entityKind() ==
                                EntityKind::Living);
        tasks_.setControlFlag(1, flag);
        tasks_.setControlFlag(4, flag);
        tasks_.setControlFlag(2, flag);
    }
}

void Mob::updateEntityActionState()
{
    ++idleTime_;
    despawnEntity();
    senses_.clearSensingCache();
    targetTasks_.tick(*this);
    tasks_.tick(*this);

    navigator_.settings().width = width_;
    navigator_.settings().height = height_;
    navigation::WorldNavigationBlockAccess access(*world_);
    navigator_.tick(access, getPositionVec(), onGround, isInWater() || isInLava());
    if (const auto target = navigator_.currentMoveTarget())
    {
        moveHelper_.setMoveTo(
            static_cast<double>(target->x),
            static_cast<double>(target->y),
            static_cast<double>(target->z),
            navigator_.speed());
    }

    moveHelper_.onUpdateMoveHelper();
    lookHelper_.onUpdateLook();
    jumpHelper_.doJump();
}

void Mob::updateRenderYawOffset()
{
    bodyHelper_.updateRenderAngles();
}

void Mob::setAIMoveSpeed(float speed) noexcept
{
    LivingEntity::setAIMoveSpeed(speed);
    setMoveForward(speed);
}

void Mob::clearDeadEntityReferences(const Entity* removed)
{
    LivingEntity::clearDeadEntityReferences(removed);
    if (attackTarget_ == removed)
        attackTarget_ = nullptr;
    if (leashHolder_ == removed)
    {
        leashed_ = false;
        leashHolder_ = nullptr;
    }
}

void Mob::despawnEntity()
{
    if (persistenceRequired_)
    {
        idleTime_ = 0;
        return;
    }
    PlayerEntity* player = world_->getClosestPlayer(posX, posY, posZ, -1.0);
    if (player == nullptr)
        return;
    const double d3 = getDistanceSq(*player);
    if (canDespawn() && d3 > 16384.0)
        setDead();
    if (idleTime_ > 600 && rand_.nextInt(800) == 0 && d3 > 1024.0 && canDespawn())
        setDead();
    else if (d3 < 1024.0)
        idleTime_ = 0;
}

void Mob::setAttackTarget(LivingEntity* target)
{
    attackTarget_ = target;
}

bool Mob::attackEntityAsMob(Entity& target)
{
    float damage = 2.0f;
    if (AttributeInstance* instance =
            attributes_.getAttributeInstance(SharedMonsterAttributes::ATTACK_DAMAGE))
        damage = static_cast<float>(instance->getAttributeValue());
    const bool hit = target.attackEntityFrom(
        DamageSource::causeMobDamage(*this), damage);
    if (hit)
    {
        if (auto* living = dynamic_cast<LivingEntity*>(&target))
            setLastAttackedEntity(living);
        swingArm();
    }
    return hit;
}

bool Mob::getCanSpawnHere()
{
    const int x = floorInt(posX);
    const int y = floorInt(boundingBox_.minY) - 1;
    const int z = floorInt(posZ);
    return world_->isSolidBlock(x, y, z);
}

bool Mob::isNotColliding() const
{
    return world_->getCollisionBoxes(this, boundingBox_).empty() &&
           world_->getEntitiesInAABB(boundingBox_, this).empty();
}

void Mob::onInitialSpawn()
{
    AttributeInstance& follow =
        getEntityAttribute(SharedMonsterAttributes::FOLLOW_RANGE);
    follow.applyModifier(AttributeModifier(
        EntityUuid::random(),
        "Random spawn bonus",
        rand_.nextGaussian() * 0.05,
        AttributeModifier::Operation::MultiplyBase,
        false));
}

core::ResourceLocation Mob::getTexture() const
{
    return core::ResourceLocation("minecraft:entity/zombie/zombie");
}

core::ResourceLocation Mob::getOverlayTexture() const
{
    return core::ResourceLocation("minecraft:entity/empty");
}

void Mob::setLeashed(bool value, Entity* holder)
{
    leashed_ = value;
    leashHolder_ = holder;
}

void Mob::updateLeashedState()
{
    if (!leashed_ || leashHolder_ == nullptr)
        return;
    if (leashHolder_->isDead() || isDead())
    {
        leashed_ = false;
        leashHolder_ = nullptr;
        return;
    }
    const double dist = getDistanceSq(*leashHolder_);
    if (dist > 10.0 * 10.0)
    {
        leashed_ = false;
        leashHolder_ = nullptr;
        return;
    }
    if (dist > 6.0 * 6.0)
    {
        const double dx = leashHolder_->posX - posX;
        const double dy = leashHolder_->posY - posY;
        const double dz = leashHolder_->posZ - posZ;
        const double len = std::sqrt(dist);
        addVelocity(dx / len * 0.4, dy / len * 0.4, dz / len * 0.4);
    }
}

float Mob::getPathPriority(navigation::PathNodeType type) const
{
    return pathPriorities_[static_cast<int>(type)];
}

void Mob::setPathPriority(navigation::PathNodeType type, float value)
{
    pathPriorities_[static_cast<int>(type)] = value;
}

float Mob::getBlockPathWeight(int, int, int) const
{
    return 0.0f;
}

float Mob::getBrightness() const
{
    const int x = floorInt(posX);
    const int y = floorInt(posY + getEyeHeight());
    const int z = floorInt(posZ);
    return world_->getLightBrightness(x, y, z);
}

void Mob::faceEntity(Entity& entity, float maxYaw, float maxPitch)
{
    const double dx = entity.posX - posX;
    const double dz = entity.posZ - posZ;
    const double dy = (entity.entityKind() == EntityKind::Living
                           ? entity.posY + entity.getEyeHeight()
                           : (entity.getEntityBoundingBox().minY +
                              entity.getEntityBoundingBox().maxY) * 0.5) -
                      (posY + getEyeHeight());
    const float yaw = toDegrees(static_cast<float>(std::atan2(dz, dx))) - 90.0f;
    const float horiz = static_cast<float>(std::sqrt(dx * dx + dz * dz));
    const float pitch = -toDegrees(static_cast<float>(std::atan2(dy, horiz)));
    rotationPitch = approachDegrees(rotationPitch, pitch, maxPitch);
    rotationYaw = approachDegrees(rotationYaw, yaw, maxYaw);
}

float Mob::interpolatedYaw(float partialTick) const
{
    const float delta = wrapDegrees(renderYawOffset - prevRenderYawOffset);
    return toRadians(prevRenderYawOffset + delta * partialTick);
}

Mob::PoseState Mob::poseState(float partialTick) const
{
    const float p = std::clamp(partialTick, 0.0f, 1.0f);
    const float headNow = wrapDegrees(rotationYawHead - renderYawOffset);
    const float headPrev = wrapDegrees(prevRotationYawHead - prevRenderYawOffset);
    const float headYaw = headPrev + wrapDegrees(headNow - headPrev) * p;
    const float pitch = prevRotationPitch +
        wrapDegrees(rotationPitch - prevRotationPitch) * p;
    return {
        static_cast<float>(ticksExisted_) + p,
        limbSwing + limbSwingAmount * p,
        prevLimbSwingAmount + (limbSwingAmount - prevLimbSwingAmount) * p,
        toRadians(headYaw),
        toRadians(pitch),
        getAttackProgress(),
        onGround ? 0.0f : std::clamp(static_cast<float>(std::abs(motionY)) * 2.0f, 0.0f, 1.0f),
        static_cast<float>(hurtTime) / 10.0f,
        std::clamp((static_cast<float>(deathTime) + p) / 20.0f, 0.0f, 1.0f),
        onGround,
        isInWater(),
        isAggressive(),
        isChild(),
        isSitting(),
        isBegging()
    };
}
}
