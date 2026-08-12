#include "entity/helpers/MoveHelper.h"

#include "entity/Mob.h"
#include "entity/Math.h"

#include <cmath>

namespace mc::entity
{
MoveHelper::MoveHelper(Mob& owner) : owner_(&owner) {}

bool MoveHelper::isUpdating() const noexcept
{
    return action_ == Action::MoveTo;
}

void MoveHelper::setMoveTo(double x, double y, double z, double speed)
{
    posX_ = x;
    posY_ = y;
    posZ_ = z;
    speed_ = speed;
    action_ = Action::MoveTo;
}

void MoveHelper::strafe(float forward, float strafe)
{
    action_ = Action::Strafe;
    moveForward_ = forward;
    moveStrafe_ = strafe;
    speed_ = 0.25;
}

float MoveHelper::limitAngle(float source, float target, float maximum)
{
    float delta = wrapDegrees(target - source);
    delta = std::clamp(delta, -maximum, maximum);
    float result = source + delta;
    if (result < 0.0f)
        result += 360.0f;
    else if (result > 360.0f)
        result -= 360.0f;
    return result;
}

void MoveHelper::onUpdateMoveHelper()
{
    if (action_ == Action::Strafe)
    {
        const float attr = static_cast<float>(
            owner_->getEntityAttribute(SharedMonsterAttributes::MOVEMENT_SPEED)
                .getAttributeValue());
        float speed = static_cast<float>(speed_) * attr;
        float forward = moveForward_;
        float strafe = moveStrafe_;
        float length = std::sqrt(forward * forward + strafe * strafe);
        if (length < 1.0f)
            length = 1.0f;
        length = speed / length;
        forward *= length;
        strafe *= length;
        owner_->setAIMoveSpeed(speed);
        owner_->setMoveForward(moveForward_);
        owner_->setMoveStrafing(moveStrafe_);
        action_ = Action::Wait;
    }
    else if (action_ == Action::MoveTo)
    {
        action_ = Action::Wait;
        const double dx = posX_ - owner_->posX;
        const double dz = posZ_ - owner_->posZ;
        const double dy = posY_ - owner_->posY;
        const double distSq = dx * dx + dy * dy + dz * dz;
        if (distSq < 2.500000277905201e-7)
        {
            owner_->setMoveForward(0.0f);
            return;
        }
        const float yaw = toDegrees(static_cast<float>(std::atan2(dz, dx))) - 90.0f;
        owner_->rotationYaw = limitAngle(owner_->rotationYaw, yaw, 90.0f);
        owner_->setAIMoveSpeed(static_cast<float>(
            speed_ * owner_->getEntityAttribute(
                SharedMonsterAttributes::MOVEMENT_SPEED).getAttributeValue()));
        owner_->setMoveForward(1.0f);
        if (dy > owner_->stepHeight &&
            dx * dx + dz * dz < static_cast<double>(std::max(1.0f, owner_->getWidth())))
        {
            owner_->getJumpHelper().setJumping();
            action_ = Action::Jumping;
        }
    }
    else if (action_ == Action::Jumping)
    {
        owner_->setAIMoveSpeed(static_cast<float>(
            speed_ * owner_->getEntityAttribute(
                SharedMonsterAttributes::MOVEMENT_SPEED).getAttributeValue()));
        if (owner_->onGround)
            action_ = Action::Wait;
    }
    else
    {
        owner_->setMoveForward(0.0f);
    }
}
}
