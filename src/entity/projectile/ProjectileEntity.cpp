#include "entity/projectile/ProjectileEntity.h"

#include "World.h"
#include "entity/LivingEntity.h"
#include "entity/Math.h"

#include <cmath>

namespace mc::entity
{
ProjectileEntity::ProjectileEntity(World& world, LivingEntity* shooter)
    : Entity(world), shooter_(shooter)
{
    setSize(0.25f, 0.25f);
    if (shooter)
    {
        setPosition(
            shooter->posX,
            shooter->posY + shooter->getEyeHeight() - 0.1,
            shooter->posZ);
        rotationYaw = shooter->rotationYaw;
        rotationPitch = shooter->rotationPitch;
    }
}

void ProjectileEntity::shoot(
    double x, double y, double z, float velocity, float inaccuracy)
{
    const double len = std::sqrt(x * x + y * y + z * z);
    x /= len;
    y /= len;
    z /= len;
    x += rand_.nextGaussian() * 0.007499999832361937 * inaccuracy;
    y += rand_.nextGaussian() * 0.007499999832361937 * inaccuracy;
    z += rand_.nextGaussian() * 0.007499999832361937 * inaccuracy;
    x *= velocity;
    y *= velocity;
    z *= velocity;
    motionX = x;
    motionY = y;
    motionZ = z;
    rotationYaw = toDegrees(static_cast<float>(std::atan2(x, z)));
    rotationPitch = toDegrees(static_cast<float>(
        std::atan2(y, std::sqrt(x * x + z * z))));
}

DamageSource ProjectileEntity::makeDamageSource()
{
    return DamageSource::causeThrownDamage(*this, shooter_);
}

void ProjectileEntity::onHitEntity(LivingEntity& entity)
{
    entity.attackEntityFrom(makeDamageSource(), damage_);
    setDead();
}

void ProjectileEntity::onHitBlock(int, int, int)
{
    setDead();
}

void ProjectileEntity::onUpdate()
{
    Entity::onUpdate();
    if (isDead())
        return;
    ++ticksInAir_;
    const AxisAlignedBB sweep = boundingBox_.expand(motionX, motionY, motionZ);
    for (Entity* other : world_->getEntitiesInAABB(sweep, this))
    {
        if (other == shooter_ && ticksInAir_ < 5)
            continue;
        if (auto* living = dynamic_cast<LivingEntity*>(other))
        {
            if (living->isAlive())
            {
                onHitEntity(*living);
                return;
            }
        }
    }
    if (!world_->getCollisionBoxes(this, sweep).empty())
    {
        onHitBlock(floorInt(posX), floorInt(posY), floorInt(posZ));
        return;
    }
    posX += motionX;
    posY += motionY;
    posZ += motionZ;
    setPosition(posX, posY, posZ);
    motionY -= 0.05;
    motionX *= 0.99;
    motionY *= 0.99;
    motionZ *= 0.99;
    if (ticksInAir_ > 1200)
        setDead();
}
}
