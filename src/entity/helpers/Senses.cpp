#include "entity/helpers/Senses.h"

#include "entity/Mob.h"

#include <algorithm>

namespace mc::entity
{
EntitySenses::EntitySenses(Mob& owner) : owner_(&owner) {}

void EntitySenses::clearSensingCache()
{
    seen_.clear();
    unseen_.clear();
}

bool EntitySenses::canSee(Entity& entity)
{
    if (std::find(seen_.begin(), seen_.end(), &entity) != seen_.end())
        return true;
    if (std::find(unseen_.begin(), unseen_.end(), &entity) != unseen_.end())
        return false;
    const bool visible = owner_->canEntityBeSeen(entity);
    if (visible)
        seen_.push_back(&entity);
    else
        unseen_.push_back(&entity);
    return visible;
}
}
