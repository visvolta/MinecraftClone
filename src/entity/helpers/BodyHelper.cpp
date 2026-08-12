#include "entity/helpers/BodyHelper.h"

#include "entity/Mob.h"
#include "entity/Math.h"

#include <cmath>

namespace mc::entity
{
BodyHelper::BodyHelper(Mob& owner) : owner_(&owner) {}

void BodyHelper::updateRenderAngles()
{
    const double dx = owner_->posX - owner_->prevPosX;
    const double dz = owner_->posZ - owner_->prevPosZ;

    if (dx * dx + dz * dz > 2.500000277905201e-7)
    {
        // Vanilla: body follows rotationYaw (move-helper facing), then the
        // head is clamped to stay within 75 degrees of the body. Using the
        // noisy position delta here made idle gravity jitter spin the model.
        owner_->renderYawOffset = owner_->rotationYaw;
        owner_->rotationYawHead = computeAngleWithBound(
            owner_->renderYawOffset, owner_->rotationYawHead, 75.0f);
        prevRenderYawHead_ = owner_->rotationYawHead;
        rotationTick_ = 0;
        return;
    }

    if (owner_->isBeingRidden() &&
        owner_->getControllingPassenger() != nullptr &&
        owner_->getControllingPassenger()->entityKind() == EntityKind::Living)
        return;

    float max = 75.0f;
    if (std::abs(owner_->rotationYawHead - prevRenderYawHead_) > 15.0f)
    {
        rotationTick_ = 0;
        prevRenderYawHead_ = owner_->rotationYawHead;
    }
    else
    {
        ++rotationTick_;
        if (rotationTick_ > 10)
        {
            max = std::max(
                1.0f - static_cast<float>(rotationTick_ - 10) / 10.0f,
                0.0f) * 75.0f;
        }
    }
    owner_->renderYawOffset = computeAngleWithBound(
        owner_->rotationYawHead, owner_->renderYawOffset, max);
}
}
