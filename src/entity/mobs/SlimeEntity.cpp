#include "entity/mobs/SlimeEntity.h"

#include "World.h"
#include "entity/Math.h"
#include "entity/PlayerEntity.h"
#include "entity/ai/VanillaGoals.h"
#include "worldgen/Biome.h"

namespace mc::entity
{
SlimeEntity::SlimeEntity(World& world) : Mob(world)
{
    setSlimeSize(1, true);
}

void SlimeEntity::applyEntityAttributes()
{
    Mob::applyEntityAttributes();
}

void SlimeEntity::initEntityAI()
{
    tasks_.add(1, std::make_unique<ai::SwimGoal>(*this));
    tasks_.add(5, std::make_unique<ai::WanderAvoidWaterGoal>(*this, 1.0, 1.0f / 20.0f));
    targetTasks_.add(1, std::make_unique<ai::NearestAttackableTargetGoal>(*this, true));
}

void SlimeEntity::setSlimeSize(int size, bool resetHealth)
{
    size_ = std::max(1, size);
    setSize(0.51000005f * static_cast<float>(size_), 0.51000005f * static_cast<float>(size_));
    setPosition(posX, posY, posZ);
    getEntityAttribute(SharedMonsterAttributes::MAX_HEALTH)
        .setBaseValue(static_cast<double>(size_ * size_));
    getEntityAttribute(SharedMonsterAttributes::MOVEMENT_SPEED)
        .setBaseValue(0.2 + 0.1 * static_cast<double>(size_));
    if (resetHealth)
        setHealth(getMaxHealth());
    experienceValue_ = size_;
}

float SlimeEntity::getEyeHeight() const
{
    return 0.325f * static_cast<float>(size_);
}

void SlimeEntity::onInitialSpawn()
{
    Mob::onInitialSpawn();
    const int size = 1 << rand_.nextInt(3);
    setSlimeSize(size, true);
}

void SlimeEntity::onUpdate()
{
    if (world_->getDifficulty() == Difficulty::Peaceful && size_ > 0)
    {
        setDead();
        return;
    }
    prevSquishFactor_ = squishFactor_;
    squishFactor_ += (squishAmount_ - squishFactor_) * 0.5f;
    Mob::onUpdate();
    if (onGround && !wasOnGround_)
        squishAmount_ = -0.5f;
    else if (!onGround && wasOnGround_)
        squishAmount_ = 1.0f;
    wasOnGround_ = onGround;
    squishAmount_ *= 0.6f;

    if (isDead() && size_ > 1)
    {
        const int smaller = size_ / 2;
        for (int i = 0; i < 2 + rand_.nextInt(3); ++i)
        {
            auto child = std::make_unique<SlimeEntity>(*world_);
            child->setSlimeSize(smaller, true);
            child->setLocationAndAngles(
                posX + (rand_.nextDouble() - 0.5) * size_,
                posY + 0.5,
                posZ + (rand_.nextDouble() - 0.5) * size_,
                rand_.nextFloat() * 360.0f, 0.0f);
            world_->spawnEntity(std::move(child));
        }
    }
}

void SlimeEntity::onCollideWithPlayer(PlayerEntity& player)
{
    dealDamage(player);
}

void SlimeEntity::dealDamage(LivingEntity& entity)
{
    const double reach = 0.6 * size_ * 0.6 * size_;
    if (canEntityBeSeen(entity) && getDistanceSq(entity) < reach)
        entity.attackEntityFrom(
            DamageSource::causeMobDamage(*this),
            static_cast<float>(getAttackStrength()));
}

bool SlimeEntity::getCanSpawnHere()
{
    if (world_->getDifficulty() == Difficulty::Peaceful)
        return false;
    const int x = floorInt(posX);
    const int y = floorInt(posY);
    const int z = floorInt(posZ);
    const BiomeId biome = world_->getBiomeAt(x, z);
    if (biome == VanillaBiomes::Swampland && y > 50 && y < 70 &&
        world_->getBlockLightLevel(x, y, z) <= 7 && rand_.nextInt(2) == 0)
        return Mob::isNotColliding();
    if (y >= 40)
        return false;
    const std::int64_t chunkX = x >= 0 ? x / 16 : (x - 15) / 16;
    const std::int64_t chunkZ = z >= 0 ? z / 16 : (z - 15) / 16;
    JavaRandom slimeRandom(
        (static_cast<std::int64_t>(world_->getSeed()) +
         chunkX * chunkX * 4987142LL + chunkX * 5947611LL +
         chunkZ * chunkZ * 4392871LL + chunkZ * 389711LL) ^ 987234911LL);
    return slimeRandom.nextInt(10) == 0 && Mob::isNotColliding();
}

core::ResourceLocation SlimeEntity::getType() const
{
    return core::ResourceLocation("minecraft:slime");
}
core::ResourceLocation SlimeEntity::getLootTable() const
{
    return core::ResourceLocation("minecraft:entities/slime");
}
gameplay::MobModelKind SlimeEntity::getModelKind() const
{
    return gameplay::MobModelKind::Slime;
}
core::ResourceLocation SlimeEntity::getTexture() const
{
    return core::ResourceLocation("minecraft:entity/slime/slime");
}
}
