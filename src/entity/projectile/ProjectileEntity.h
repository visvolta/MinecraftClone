#pragma once

#include "entity/Entity.h"

namespace mc::entity
{
class ProjectileEntity : public Entity
{
public:
    ProjectileEntity(World& world, LivingEntity* shooter);

    void onUpdate() override;
    [[nodiscard]] EntityKind entityKind() const noexcept override
    {
        return EntityKind::Projectile;
    }
    [[nodiscard]] LivingEntity* getShooter() const noexcept { return shooter_; }
    void shoot(double x, double y, double z, float velocity, float inaccuracy);
    void setDamage(float damage) noexcept { damage_ = damage; }

protected:
    virtual void onHitEntity(LivingEntity& entity);
    virtual void onHitBlock(int x, int y, int z);
    [[nodiscard]] virtual DamageSource makeDamageSource();

    LivingEntity* shooter_ = nullptr;
    float damage_ = 2.0f;
    int ticksInAir_ = 0;
};
}
