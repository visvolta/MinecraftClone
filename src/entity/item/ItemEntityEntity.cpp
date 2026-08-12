#include "entity/item/ItemEntityEntity.h"

#include "Inventory.h"
#include "World.h"
#include "entity/PlayerEntity.h"

namespace mc::entity
{
ItemEntityEntity::ItemEntityEntity(
    World& world, double x, double y, double z, ItemStack stack)
    : Entity(world), stack_(stack)
{
    setSize(0.25f, 0.25f);
    setPosition(x, y, z);
    hoverStart_ = static_cast<float>(rand_.nextFloat() * 3.1415926f * 2.0f);
}

core::ResourceLocation ItemEntityEntity::getType() const
{
    return core::ResourceLocation("minecraft:item");
}

void ItemEntityEntity::onUpdate()
{
    Entity::onUpdate();
    if (pickupDelay_ > 0)
        --pickupDelay_;
    motionY -= 0.04;
    move(MoverType::Self, motionX, motionY, motionZ);
    const float drag = onGround ? 0.58800006f : 0.98f;
    motionX *= drag;
    motionY *= 0.98;
    motionZ *= drag;
    if (onGround)
        motionY *= -0.5;
    if (ticksExisted_ >= 6000 || stack_.empty())
        setDead();
}

void ItemEntityEntity::onCollideWithPlayer(PlayerEntity& player)
{
    if (pickupDelay_ > 0 || !player.isAlive())
        return;
    if (getDistanceSq(player) > 2.25)
        return;
    // Pickup is handled by ClientApplication via ItemEntityManager for now.
}
}
