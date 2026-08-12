#include "entity/TameableEntity.h"

#include "World.h"
#include "entity/PlayerEntity.h"

namespace mc::entity
{
TameableEntity::TameableEntity(World& world) : AnimalEntity(world) {}

void TameableEntity::setTamed(bool tamed)
{
    tamed_ = tamed;
}

PlayerEntity* TameableEntity::getOwner() const
{
    if (ownerId_.empty())
        return nullptr;
    if (PlayerEntity* player = world_->getPlayer())
        if (player->uuid() == ownerId_)
            return player;
    return nullptr;
}

bool TameableEntity::isOwner(const LivingEntity& entity) const
{
    return entity.uuid() == ownerId_;
}
}
