#include "entity/MonsterEntity.h"

#include "World.h"
#include "entity/Math.h"

namespace mc::entity
{
MonsterEntity::MonsterEntity(World& world) : Creature(world)
{
    experienceValue_ = 5;
}

void MonsterEntity::applyEntityAttributes()
{
    Creature::applyEntityAttributes();
    attributes_.registerAttribute(SharedMonsterAttributes::ATTACK_DAMAGE);
}

void MonsterEntity::onLivingUpdate()
{
    updateArmSwingProgress();
    const float brightness = getBrightness();
    if (brightness > 0.5f)
        idleTime_ += 2;
    Creature::onLivingUpdate();
}

void MonsterEntity::onUpdate()
{
    Creature::onUpdate();
    if (!isDead() && world_->getDifficulty() == Difficulty::Peaceful)
        setDead();
}

bool MonsterEntity::isValidLightLevel()
{
    const int x = floorInt(posX);
    const int y = floorInt(boundingBox_.minY);
    const int z = floorInt(posZ);
    const int sky = world_->getEffectiveSkyLight(x, y, z);
    const int neighbor = world_->getLightFromNeighbors(x, y, z);
    return vanillaHostileLightAllowsSpawn(
        sky, neighbor, rand_.nextInt(32), rand_.nextInt(8));
}

bool MonsterEntity::getCanSpawnHere()
{
    return world_->getDifficulty() != Difficulty::Peaceful &&
           isValidLightLevel() && Creature::getCanSpawnHere();
}

float MonsterEntity::getBlockPathWeight(int x, int y, int z) const
{
    return 0.5f - world_->getLightBrightness(x, y, z);
}

bool MonsterEntity::attackEntityAsMob(Entity& target)
{
    float damage = static_cast<float>(
        getEntityAttribute(SharedMonsterAttributes::ATTACK_DAMAGE)
            .getAttributeValue());
    return target.attackEntityFrom(DamageSource::causeMobDamage(*this), damage);
}
}
