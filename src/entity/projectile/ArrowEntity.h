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

class ThrownPotionEntity : public ProjectileEntity
{
public:
    ThrownPotionEntity(World& world, LivingEntity* shooter);
    [[nodiscard]] core::ResourceLocation getType() const override;

protected:
    void onHitEntity(LivingEntity& entity) override;
    void onHitBlock(int x, int y, int z) override;
};

class LlamaSpitEntity : public ProjectileEntity
{
public:
    LlamaSpitEntity(World& world, LivingEntity* shooter);
    [[nodiscard]] core::ResourceLocation getType() const override;
};
}
