#include "entity/Creature.h"

#include "World.h"
#include "entity/Math.h"

#include <cmath>

namespace mc::entity
{
Creature::Creature(World& world) : Mob(world) {}

float Creature::getBlockPathWeight(int, int, int) const
{
    return 0.0f;
}

bool Creature::getCanSpawnHere()
{
    return Mob::getCanSpawnHere() && getBlockPathWeight(
        static_cast<int>(std::floor(posX)),
        static_cast<int>(std::floor(posY)),
        static_cast<int>(std::floor(posZ))) >= 0.0f;
}

bool Creature::hasPath() const
{
    return !navigator_.noPath();
}

void Creature::setHomePosAndDistance(int x, int y, int z, int distance)
{
    homeX_ = x;
    homeY_ = y;
    homeZ_ = z;
    maximumHomeDistance_ = static_cast<float>(distance);
}

bool Creature::isWithinHomeDistanceCurrentPosition() const
{
    if (maximumHomeDistance_ == -1.0f)
        return true;
    const double dx = posX - homeX_;
    const double dy = posY - homeY_;
    const double dz = posZ - homeZ_;
    return dx * dx + dy * dy + dz * dz <
        static_cast<double>(maximumHomeDistance_ * maximumHomeDistance_);
}

void Creature::detachHome()
{
    maximumHomeDistance_ = -1.0f;
}
}
