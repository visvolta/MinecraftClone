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
        owner_->renderYawOffset = toDegrees(
            static_cast<float>(std::atan2(dz, dx))) - 90.0f;
        owner_->rotationYawHead = owner_->renderYawOffset;
        rotationTick_ = 0;
    }
    else
    {
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
                max = std::max(1.0f - static_cast<float>(rotationTick_ - 10) / 10.0f, 0.0f) * 75.0f;
        }
        owner_->renderYawOffset = approachDegrees(
            owner_->renderYawOffset, owner_->rotationYawHead, max);
    }
}
}
