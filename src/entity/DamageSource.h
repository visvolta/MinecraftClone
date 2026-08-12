#pragma once

#include <string>

namespace mc::entity
{
class Entity;
class LivingEntity;

class DamageSource
{
public:
    static const DamageSource IN_FIRE;
    static const DamageSource LIGHTNING_BOLT;
    static const DamageSource ON_FIRE;
    static const DamageSource LAVA;
    static const DamageSource HOT_FLOOR;
    static const DamageSource IN_WALL;
    static const DamageSource CRAMMING;
    static const DamageSource DROWN;
    static const DamageSource STARVE;
    static const DamageSource CACTUS;
    static const DamageSource FALL;
    static const DamageSource FLY_INTO_WALL;
    static const DamageSource OUT_OF_WORLD;
    static const DamageSource GENERIC;
    static const DamageSource MAGIC;
    static const DamageSource WITHER;
    static const DamageSource ANVIL;
    static const DamageSource FALLING_BLOCK;
    static const DamageSource DRAGON_BREATH;
    static const DamageSource FIREWORKS;

    explicit DamageSource(const char* damageType) noexcept;

    [[nodiscard]] static DamageSource causeMobDamage(LivingEntity& mob);
    [[nodiscard]] static DamageSource causePlayerDamage(LivingEntity& player);
    [[nodiscard]] static DamageSource causeThrownDamage(
        Entity& immediate,
        LivingEntity* owner
    );
    [[nodiscard]] static DamageSource causeIndirectMagicDamage(
        Entity& immediate,
        LivingEntity* owner
    );
    [[nodiscard]] static DamageSource causeThornsDamage(LivingEntity& source);
    [[nodiscard]] static DamageSource causeExplosionDamage(LivingEntity* source);
    [[nodiscard]] static DamageSource causeArrowDamage(
        Entity& arrow,
        LivingEntity* shooter
    );

    [[nodiscard]] const char* getDamageType() const noexcept { return damageType_; }
    [[nodiscard]] Entity* getImmediateSource() const noexcept { return immediate_; }
    [[nodiscard]] LivingEntity* getTrueSource() const noexcept { return trueSource_; }
    [[nodiscard]] bool isFireDamage() const noexcept { return fireDamage_; }
    [[nodiscard]] bool isExplosion() const noexcept { return explosion_; }
    [[nodiscard]] bool isProjectile() const noexcept { return projectile_; }
    [[nodiscard]] bool isMagicDamage() const noexcept { return magicDamage_; }
    [[nodiscard]] bool isUnblockable() const noexcept { return unblockable_; }
    [[nodiscard]] bool isDamageAbsolute() const noexcept { return absolute_; }
    [[nodiscard]] bool canHarmInCreative() const noexcept { return creativePlayer_; }
    [[nodiscard]] float getHungerDamage() const noexcept { return hungerDamage_; }

    DamageSource& setFireDamage() noexcept;
    DamageSource& setExplosion() noexcept;
    DamageSource& setProjectile() noexcept;
    DamageSource& setMagicDamage() noexcept;
    DamageSource& setDamageBypassesArmor() noexcept;
    DamageSource& setDamageIsAbsolute() noexcept;
    DamageSource& setDamageAllowedInCreativeMode() noexcept;

private:
    const char* damageType_ = "generic";
    Entity* immediate_ = nullptr;
    LivingEntity* trueSource_ = nullptr;
    bool fireDamage_ = false;
    bool explosion_ = false;
    bool projectile_ = false;
    bool magicDamage_ = false;
    bool unblockable_ = false;
    bool absolute_ = false;
    bool creativePlayer_ = false;
    float hungerDamage_ = 0.1f;
};
}
