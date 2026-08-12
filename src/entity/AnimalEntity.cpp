#include "entity/AnimalEntity.h"

#include "World.h"
#include "entity/Math.h"
#include "entity/PlayerEntity.h"
#include "entity/item/XpOrbEntity.h"

namespace mc::entity
{
AnimalEntity::AnimalEntity(World& world) : AgeableEntity(world)
{
    experienceValue_ = 1 + rand_.nextInt(3);
}

bool AnimalEntity::getCanSpawnHere()
{
    const int x = floorInt(posX);
    const int y = floorInt(boundingBox_.minY);
    const int z = floorInt(posZ);
    return world_->getBlock(x, y - 1, z) == BlockType::Grass &&
           world_->getSkyLightLevel(x, y, z) > 8 &&
           Creature::getCanSpawnHere();
}

float AnimalEntity::getBlockPathWeight(int x, int y, int z) const
{
    return world_->getBlock(x, y - 1, z) == BlockType::Grass ? 10.0f
        : world_->getLightBrightness(x, y, z) - 0.5f;
}

bool AnimalEntity::isBreedingItem(ItemType item) const
{
    return item == ItemType::WheatItem;
}

void AnimalEntity::setInLove(PlayerEntity* player)
{
    inLove_ = 600;
    if (player)
        loveCause_ = player->uuid();
}

bool AnimalEntity::canMateWith(AnimalEntity& other) const
{
    if (&other == this)
        return false;
    return getType() == other.getType() && isInLove() && other.isInLove();
}

void AnimalEntity::spawnChildFromBreeding(AnimalEntity& mate)
{
    auto child = createChild(mate);
    if (!child)
        return;
    setGrowingAge(6000);
    mate.setGrowingAge(6000);
    resetInLove();
    mate.resetInLove();
    child->setGrowingAge(-24000);
    child->setLocationAndAngles(posX, posY, posZ, rotationYaw, rotationPitch);
    world_->spawnEntity(std::move(child));
    world_->spawnXpOrbs(posX, posY, posZ, 1 + rand_.nextInt(7));
}

bool AnimalEntity::processInteract(PlayerEntity& player, ItemStack& stack)
{
    if (stack.empty())
        return false;
    if (isBreedingItem(stack.item))
    {
        if (isChild())
        {
            addGrowth(static_cast<int>(
                static_cast<float>(-growingAge_ / 20) * 0.1f));
            if (stack.count > 1)
                --stack.count;
            else
                stack.clear();
            return true;
        }
        if (growingAge_ == 0 && inLove_ <= 0)
        {
            setInLove(&player);
            if (stack.count > 1)
                --stack.count;
            else
                stack.clear();
            return true;
        }
    }
    return false;
}

void AnimalEntity::onLivingUpdate()
{
    AgeableEntity::onLivingUpdate();
    if (getGrowingAge() != 0)
        inLove_ = 0;
    if (inLove_ > 0)
        --inLove_;
}
}
