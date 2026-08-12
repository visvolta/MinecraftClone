#include "entity/mobs/WitchEntity.h"

#include "World.h"
#include "entity/ai/VanillaGoals.h"
#include "entity/projectile/ArrowEntity.h"

#include <cmath>

namespace mc::entity
{
WitchEntity::WitchEntity(World& world) : MonsterEntity(world)
{
    setSize(0.6f, 1.95f);
    experienceValue_ = 5;
}

void WitchEntity::initEntityAI()
{
    tasks_.add(1, std::make_unique<ai::SwimGoal>(*this));
    tasks_.add(2, std::make_unique<ai::AttackRangedBowGoal>(*this, 1.0, 60, 10.0f));
    tasks_.add(2, std::make_unique<ai::WanderAvoidWaterGoal>(*this, 1.0));
    tasks_.add(3, std::make_unique<ai::WatchClosestGoal>(*this, 8.0f));
    tasks_.add(3, std::make_unique<ai::LookIdleGoal>(*this));
    targetTasks_.add(1, std::make_unique<ai::HurtByTargetGoal>(*this, false));
    targetTasks_.add(2, std::make_unique<ai::NearestAttackableTargetGoal>(*this, true));
}

void WitchEntity::applyEntityAttributes()
{
    MonsterEntity::applyEntityAttributes();
    getEntityAttribute(SharedMonsterAttributes::MAX_HEALTH).setBaseValue(26.0);
    getEntityAttribute(SharedMonsterAttributes::MOVEMENT_SPEED).setBaseValue(0.25);
}

bool WitchEntity::attackEntityAsMob(Entity& target)
{
    auto potion = std::make_unique<ThrownPotionEntity>(getWorld(), this);
    const double dx = target.posX + target.motionX - posX;
    const double dy = target.posY + target.getEyeHeight() - 1.1 - potion->posY;
    const double dz = target.posZ + target.motionZ - posZ;
    const float dist = static_cast<float>(std::sqrt(dx * dx + dz * dz));
    potion->shoot(dx, dy + dist * 0.2, dz, 0.75f, 8.0f);
    getWorld().spawnEntity(std::move(potion));
    return true;
}

core::ResourceLocation WitchEntity::getType() const
{
    return core::ResourceLocation("minecraft:witch");
}
core::ResourceLocation WitchEntity::getLootTable() const
{
    return core::ResourceLocation("minecraft:entities/witch");
}
gameplay::MobModelKind WitchEntity::getModelKind() const
{
    return gameplay::MobModelKind::Witch;
}
core::ResourceLocation WitchEntity::getTexture() const
{
    return core::ResourceLocation("minecraft:entity/witch");
}
}
