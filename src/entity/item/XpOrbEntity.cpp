#include "entity/item/XpOrbEntity.h"

#include "World.h"
#include "entity/PlayerEntity.h"

namespace mc::entity
{
XpOrbEntity::XpOrbEntity(World& world, double x, double y, double z, int xpValue)
    : Entity(world), xpValue_(std::max(1, xpValue))
{
    setSize(0.5f, 0.5f);
    setPosition(x, y, z);
    motionX = (rand_.nextDouble() * 0.2 - 0.1);
    motionY = 0.2;
    motionZ = (rand_.nextDouble() * 0.2 - 0.1);
}

core::ResourceLocation XpOrbEntity::getType() const
{
    return core::ResourceLocation("minecraft:xp_orb");
}

void XpOrbEntity::onUpdate()
{
    Entity::onUpdate();
    if (delayBeforeCanPickup_ > 0)
        --delayBeforeCanPickup_;
    motionY -= 0.03;
    move(MoverType::Self, motionX, motionY, motionZ);
    motionX *= 0.98;
    motionY *= 0.98;
    motionZ *= 0.98;
    if (onGround)
        motionY *= -0.5;
    if (ticksExisted_ > 6000)
        setDead();
    if (PlayerEntity* player = world_->getPlayer())
        onCollideWithPlayer(*player);
}

void XpOrbEntity::onCollideWithPlayer(PlayerEntity& player)
{
    if (delayBeforeCanPickup_ > 0 || !player.isAlive())
        return;
    if (getDistanceSq(player) > 2.25)
        return;
    player.addExperience(xpValue_);
    setDead();
}
}
