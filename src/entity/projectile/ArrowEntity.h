#pragma once

#include "entity/projectile/ProjectileEntity.h"

namespace mc::entity
{
class ArrowEntity : public ProjectileEntity
{
public:
    ArrowEntity(World& world, LivingEntity* shooter);
    [[nodiscard]] core::ResourceLocation getType() const override;
    void onUpdate() override;

protected:
    DamageSource makeDamageSource() override;
};

class SnowballEntity : public ProjectileEntity
{
public:
    SnowballEntity(World& world, LivingEntity* shooter);
    [[nodiscard]] core::ResourceLocation getType() const override;

protected:
    void onHitEntity(LivingEntity& entity) override;
};

enum class SplashPotionType
{
    Harming,
    Slowness,
    Poison,
    Weakness
};

class ThrownPotionEntity : public ProjectileEntity
{
public:
    ThrownPotionEntity(
        World& world,
        LivingEntity* shooter,
        SplashPotionType type = SplashPotionType::Harming);
    [[nodiscard]] core::ResourceLocation getType() const override;
    [[nodiscard]] SplashPotionType potionType() const noexcept { return type_; }

protected:
    void onHitEntity(LivingEntity& entity) override;
    void onHitBlock(int x, int y, int z) override;

private:
    void applyTo(LivingEntity& entity, double intensity);
    SplashPotionType type_ = SplashPotionType::Harming;
};

class LlamaSpitEntity : public ProjectileEntity
{
public:
    LlamaSpitEntity(World& world, LivingEntity* shooter);
    [[nodiscard]] core::ResourceLocation getType() const override;
};
}
