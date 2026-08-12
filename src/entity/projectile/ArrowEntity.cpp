#include "entity/projectile/ArrowEntity.h"

#include "World.h"
#include "entity/LivingEntity.h"
#include "gameplay/SurvivalStats.h"

namespace mc::entity
{
ArrowEntity::ArrowEntity(World& world, LivingEntity* shooter)
    : ProjectileEntity(world, shooter)
{
    setSize(0.5f, 0.5f);
    damage_ = 2.0f;
}

core::ResourceLocation ArrowEntity::getType() const
{
    return core::ResourceLocation("minecraft:arrow");
}

DamageSource ArrowEntity::makeDamageSource()
{
    return DamageSource::causeArrowDamage(*this, shooter_);
}

void ArrowEntity::onUpdate()
{
    ProjectileEntity::onUpdate();
}

SnowballEntity::SnowballEntity(World& world, LivingEntity* shooter)
    : ProjectileEntity(world, shooter)
{
    damage_ = 0.0f;
}

core::ResourceLocation SnowballEntity::getType() const
{
    return core::ResourceLocation("minecraft:snowball");
}

void SnowballEntity::onHitEntity(LivingEntity& entity)
{
    // Blazes take 3; other mobs take 0 but still get knockback via attack.
    float amount = 0.0f;
    if (entity.getType().path() == "blaze")
        amount = 3.0f;
    entity.attackEntityFrom(makeDamageSource(), amount);
    setDead();
}

ThrownPotionEntity::ThrownPotionEntity(World& world, LivingEntity* shooter)
    : ProjectileEntity(world, shooter)
{
    damage_ = 0.0f;
}

core::ResourceLocation ThrownPotionEntity::getType() const
{
    return core::ResourceLocation("minecraft:potion");
}

void ThrownPotionEntity::onHitEntity(LivingEntity& entity)
{
    entity.addPotionEffect({gameplay::StatusEffectType::Slowness, 180, 0});
    entity.addPotionEffect({gameplay::StatusEffectType::Poison, 100, 0});
    setDead();
}

void ThrownPotionEntity::onHitBlock(int, int, int)
{
    for (Entity* other : world_->getEntitiesInAABB(
             getEntityBoundingBox().grow(4.0, 2.0, 4.0), this))
    {
        if (auto* living = dynamic_cast<LivingEntity*>(other))
        {
            if (getDistanceSq(*living) < 16.0)
            {
                living->addPotionEffect(
                    {gameplay::StatusEffectType::Slowness, 180, 0});
                living->addPotionEffect(
                    {gameplay::StatusEffectType::Poison, 100, 0});
            }
        }
    }
    setDead();
}

LlamaSpitEntity::LlamaSpitEntity(World& world, LivingEntity* shooter)
    : ProjectileEntity(world, shooter)
{
    damage_ = 1.0f;
}

core::ResourceLocation LlamaSpitEntity::getType() const
{
    return core::ResourceLocation("minecraft:llama_spit");
}
}
