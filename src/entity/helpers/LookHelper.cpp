#include "entity/helpers/LookHelper.h"

#include "entity/Mob.h"
#include "entity/Math.h"

#include <cmath>

namespace mc::entity
{
LookHelper::LookHelper(Mob& owner) : owner_(&owner) {}

void LookHelper::setLookPositionWithEntity(
    Entity& entity, float deltaYaw, float deltaPitch)
{
    posX_ = entity.posX;
    if (entity.entityKind() == EntityKind::Living)
        posY_ = entity.posY + entity.getEyeHeight();
    else
        posY_ = (entity.getEntityBoundingBox().minY +
                 entity.getEntityBoundingBox().maxY) * 0.5;
    posZ_ = entity.posZ;
    deltaLookYaw_ = deltaYaw;
    deltaLookPitch_ = deltaPitch;
    looking_ = true;
}

void LookHelper::setLookPosition(
    double x, double y, double z, float deltaYaw, float deltaPitch)
{
    posX_ = x;
    posY_ = y;
    posZ_ = z;
    deltaLookYaw_ = deltaYaw;
    deltaLookPitch_ = deltaPitch;
    looking_ = true;
}

float LookHelper::updateRotation(float current, float target, float maxChange)
{
    float delta = wrapDegrees(target - current);
    delta = std::clamp(delta, -maxChange, maxChange);
    return current + delta;
}

void LookHelper::onUpdateLook()
{
    owner_->rotationPitch = 0.0f;
    if (looking_)
    {
        looking_ = false;
        const double d0 = posX_ - owner_->posX;
        const double d1 = posY_ - (owner_->posY + owner_->getEyeHeight());
        const double d2 = posZ_ - owner_->posZ;
        const double d3 = std::sqrt(d0 * d0 + d2 * d2);
        const float yaw = toDegrees(static_cast<float>(std::atan2(d2, d0))) - 90.0f;
        const float pitch = -toDegrees(static_cast<float>(std::atan2(d1, d3)));
        owner_->rotationPitch = updateRotation(
            owner_->rotationPitch, pitch, deltaLookPitch_);
        owner_->rotationYawHead = updateRotation(
            owner_->rotationYawHead, yaw, deltaLookYaw_);
    }
    else
    {
        owner_->rotationYawHead = updateRotation(
            owner_->rotationYawHead, owner_->renderYawOffset, 10.0f);
    }

    const float f2 = wrapDegrees(owner_->rotationYawHead - owner_->renderYawOffset);
    if (!owner_->getNavigator().noPath())
    {
        if (f2 < -75.0f)
            owner_->rotationYawHead = owner_->renderYawOffset - 75.0f;
        if (f2 > 75.0f)
            owner_->rotationYawHead = owner_->renderYawOffset + 75.0f;
    }
}
}
