#include "entity/helpers/JumpHelper.h"

#include "entity/Mob.h"

namespace mc::entity
{
JumpHelper::JumpHelper(Mob& owner) : owner_(&owner) {}

void JumpHelper::setJumping()
{
    jumping_ = true;
}

void JumpHelper::doJump()
{
    owner_->setJumping(jumping_);
    jumping_ = false;
}
}
