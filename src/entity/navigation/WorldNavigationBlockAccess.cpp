#include "entity/navigation/PathNavigation.h"

#include "Chunk.h"
#include "World.h"

namespace mc::entity::navigation
{
WorldNavigationBlockAccess::WorldNavigationBlockAccess(
    const World& world) noexcept : world_(&world) {}

content::BlockState WorldNavigationBlockAccess::blockState(
    int x,
    int y,
    int z) const
{
    return world_->getActualBlockState(x, y, z);
}

bool WorldNavigationBlockAccess::loaded(int x, int y, int z) const
{
    return y >= 0 && y < Chunk::HEIGHT && world_->isBlockLoaded(x, y, z);
}
}
