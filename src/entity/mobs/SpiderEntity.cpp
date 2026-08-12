#include "entity/mobs/SpiderEntity.h"

#include "World.h"
#include "entity/LivingEntity.h"
#include "entity/ai/VanillaGoals.h"
#include "entity/navigation/PathNavigation.h"

namespace mc::entity
{
SpiderEntity::SpiderEntity(World& world) : MonsterEntity(world)
{
    setSize(1.4f, 0.9f);
    experienceValue_ = 5;
    navigator_.settings().kind = navigation::NavigationKind::Climbing;
}

void SpiderEntity::initEntityAI()
{
    tasks_.add(1, std::make_unique<ai::SwimGoal>(*this));
    tasks_.add(3, std::make_unique<ai::LeapAtTargetGoal>(*this, 0.4f));
    tasks_.add(4, std::make_unique<ai::AttackMeleeGoal>(*this, 1.0, false));
    tasks_.add(5, std::make_unique<ai::WanderAvoidWaterGoal>(*this, 0.8));
    tasks_.add(6, std::make_unique<ai::WatchClosestGoal>(*this, 8.0f));
    tasks_.add(6, std::make_unique<ai::LookIdleGoal>(*this));
    targetTasks_.add(1, std::make_unique<ai::HurtByTargetGoal>(*this, false));
    targetTasks_.add(2, std::make_unique<ai::NearestAttackableTargetGoal>(*this, true));
}

void SpiderEntity::applyEntityAttributes()
{
    MonsterEntity::applyEntityAttributes();
    getEntityAttribute(SharedMonsterAttributes::MAX_HEALTH).setBaseValue(16.0);
    getEntityAttribute(SharedMonsterAttributes::MOVEMENT_SPEED).setBaseValue(0.30000001192092896);
}

bool SpiderEntity::isOnLadder() const
{
    return collidedHorizontally;
}

core::ResourceLocation SpiderEntity::getType() const
{
    return core::ResourceLocation("minecraft:spider");
}
core::ResourceLocation SpiderEntity::getLootTable() const
{
    return core::ResourceLocation("minecraft:entities/spider");
}
gameplay::MobModelKind SpiderEntity::getModelKind() const
{
    return gameplay::MobModelKind::Spider;
}
core::ResourceLocation SpiderEntity::getTexture() const
{
    return core::ResourceLocation("minecraft:entity/spider/spider");
}
core::ResourceLocation SpiderEntity::getOverlayTexture() const
{
    return core::ResourceLocation("minecraft:entity/spider_eyes");
}

CaveSpiderEntity::CaveSpiderEntity(World& world) : SpiderEntity(world)
{
    setSize(0.7f, 0.5f);
}

void CaveSpiderEntity::applyEntityAttributes()
{
    SpiderEntity::applyEntityAttributes();
    getEntityAttribute(SharedMonsterAttributes::MAX_HEALTH).setBaseValue(12.0);
}

bool CaveSpiderEntity::attackEntityAsMob(Entity& target)
{
    if (SpiderEntity::attackEntityAsMob(target))
    {
        if (auto* living = dynamic_cast<LivingEntity*>(&target))
        {
            int duration = 0;
            if (world_->getDifficulty() == Difficulty::Normal)
                duration = 7;
            else if (world_->getDifficulty() == Difficulty::Hard)
                duration = 15;
            if (duration > 0)
                living->addPotionEffect(
                    {gameplay::StatusEffectType::Poison, duration * 20, 0});
        }
        return true;
    }
    return false;
}

core::ResourceLocation CaveSpiderEntity::getType() const
{
    return core::ResourceLocation("minecraft:cave_spider");
}
core::ResourceLocation CaveSpiderEntity::getLootTable() const
{
    return core::ResourceLocation("minecraft:entities/cave_spider");
}
core::ResourceLocation CaveSpiderEntity::getTexture() const
{
    return core::ResourceLocation("minecraft:entity/spider/cave_spider");
}
}
