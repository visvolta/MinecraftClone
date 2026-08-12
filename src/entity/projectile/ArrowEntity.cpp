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

ThrownPotionEntity::ThrownPotionEntity(
    World& world,
    LivingEntity* shooter,
    SplashPotionType type)
    : ProjectileEntity(world, shooter),
      type_(type)
{
    damage_ = 0.0f;
}

core::ResourceLocation ThrownPotionEntity::getType() const
{
    return core::ResourceLocation("minecraft:potion");
}

void ThrownPotionEntity::applyTo(LivingEntity& entity, double intensity)
{
    if (intensity <= 0.0)
        return;
    switch (type_)
    {
        case SplashPotionType::Harming:
            entity.attackEntityFrom(
                DamageSource::causeIndirectMagicDamage(*this, shooter_),
                static_cast<float>(6.0 * intensity));
            break;
        case SplashPotionType::Slowness:
        {
            const int duration = static_cast<int>(intensity * 1800.0 + 0.5);
            if (duration > 20)
                entity.addPotionEffect(
                    {gameplay::StatusEffectType::Slowness, duration, 0});
            break;
        }
        case SplashPotionType::Poison:
        {
            const int duration = static_cast<int>(intensity * 900.0 + 0.5);
            if (duration > 20)
                entity.addPotionEffect(
                    {gameplay::StatusEffectType::Poison, duration, 0});
            break;
        }
        case SplashPotionType::Weakness:
        {
            const int duration = static_cast<int>(intensity * 1800.0 + 0.5);
            if (duration > 20)
                entity.addPotionEffect(
                    {gameplay::StatusEffectType::Weakness, duration, 0});
            break;
        }
    }
}

void ThrownPotionEntity::onHitEntity(LivingEntity&)
{
    onHitBlock(0, 0, 0);
}

void ThrownPotionEntity::onHitBlock(int, int, int)
{
    const AxisAlignedBB area = getEntityBoundingBox().grow(4.0, 2.0, 4.0);
    for (Entity* other : world_->getEntitiesInAABB(area, this))
    {
        auto* living = dynamic_cast<LivingEntity*>(other);
        if (!living || living->isDead())
            continue;
        const double distSq = getDistanceSq(*living);
        if (distSq >= 16.0)
            continue;
        double intensity = 1.0 - std::sqrt(distSq) / 4.0;
        applyTo(*living, intensity);
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
