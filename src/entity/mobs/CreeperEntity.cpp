#include "entity/mobs/CreeperEntity.h"

#include "World.h"
#include "entity/Math.h"
#include "entity/ai/VanillaGoals.h"
#include "entity/PlayerEntity.h"

namespace mc::entity
{
CreeperEntity::CreeperEntity(World& world) : MonsterEntity(world)
{
    setSize(0.6f, 1.7f);
    experienceValue_ = 5;
}

void CreeperEntity::initEntityAI()
{
    tasks_.add(1, std::make_unique<ai::SwimGoal>(*this));
    tasks_.add(2, std::make_unique<ai::CreeperSwellGoal>(*this));
    tasks_.add(4, std::make_unique<ai::AttackMeleeGoal>(*this, 1.0, false));
    tasks_.add(5, std::make_unique<ai::WanderAvoidWaterGoal>(*this, 0.8));
    tasks_.add(6, std::make_unique<ai::WatchClosestGoal>(*this, 8.0f));
    tasks_.add(6, std::make_unique<ai::LookIdleGoal>(*this));
    targetTasks_.add(1, std::make_unique<ai::NearestAttackableTargetGoal>(*this, true));
    targetTasks_.add(2, std::make_unique<ai::HurtByTargetGoal>(*this));
}

void CreeperEntity::applyEntityAttributes()
{
    MonsterEntity::applyEntityAttributes();
    getEntityAttribute(SharedMonsterAttributes::MOVEMENT_SPEED).setBaseValue(0.25);
}

core::ResourceLocation CreeperEntity::getType() const
{
    return core::ResourceLocation("minecraft:creeper");
}

core::ResourceLocation CreeperEntity::getLootTable() const
{
    return core::ResourceLocation("minecraft:entities/creeper");
}

gameplay::MobModelKind CreeperEntity::getModelKind() const
{
    return gameplay::MobModelKind::Creeper;
}

core::ResourceLocation CreeperEntity::getTexture() const
{
    return core::ResourceLocation("minecraft:entity/creeper/creeper");
}

int CreeperEntity::getMaxFallHeight() const
{
    return getAttackTarget() == nullptr
        ? 3
        : 3 + static_cast<int>(getHealth() - 1.0f);
}

void CreeperEntity::fall(float distance, float damageMultiplier)
{
    MonsterEntity::fall(distance, damageMultiplier);
    timeSinceIgnited_ = static_cast<int>(
        static_cast<float>(timeSinceIgnited_) + distance * 1.5f);
    if (timeSinceIgnited_ > fuseTime_ - 5)
        timeSinceIgnited_ = fuseTime_ - 5;
}

float CreeperEntity::getAttackProgress() const
{
    return static_cast<float>(lastActiveTime_ +
        (timeSinceIgnited_ - lastActiveTime_)) /
        static_cast<float>(std::max(1, fuseTime_ - 2));
}

void CreeperEntity::onUpdate()
{
    if (isAlive())
    {
        lastActiveTime_ = timeSinceIgnited_;
        const int state = creeperState_;
        if (state > 0 && timeSinceIgnited_ == 0)
        {
            // primed; no audio
        }
        timeSinceIgnited_ += state;
        if (timeSinceIgnited_ < 0)
            timeSinceIgnited_ = 0;
        if (timeSinceIgnited_ >= fuseTime_)
        {
            timeSinceIgnited_ = fuseTime_;
            explode();
        }
    }
    MonsterEntity::onUpdate();
}

void CreeperEntity::explode()
{
    if (PlayerEntity* player = getWorld().getPlayer())
    {
        const double dist = getDistanceSq(*player);
        if (dist < 36.0)
        {
            float damage = 6.0f * (1.0f - static_cast<float>(std::sqrt(dist)) / 6.0f);
            if (world_->getDifficulty() == Difficulty::Hard)
                damage *= 1.5f;
            else if (world_->getDifficulty() == Difficulty::Easy)
                damage *= 0.5f;
            player->attackEntityFrom(DamageSource::causeExplosionDamage(this), damage);
        }
    }
    constexpr int radius = 3;
    const int cx = floorInt(posX);
    const int cy = floorInt(posY);
    const int cz = floorInt(posZ);
    for (int x = -radius; x <= radius; ++x)
    for (int y = -radius; y <= radius; ++y)
    for (int z = -radius; z <= radius; ++z)
    {
        if (x * x + y * y + z * z > radius * radius)
            continue;
        const BlockType block = world_->getBlock(cx + x, cy + y, cz + z);
        if (block != BlockType::Air && block != BlockType::Bedrock &&
            block != BlockType::Obsidian)
            world_->setBlock(cx + x, cy + y, cz + z, BlockType::Air);
    }
    setDead();
}
}
