#include "entity/DamageSource.h"

#include "entity/Entity.h"
#include "entity/LivingEntity.h"

namespace mc::entity
{
DamageSource::DamageSource(const char* damageType) noexcept
    : damageType_(damageType)
{
}

DamageSource& DamageSource::setFireDamage() noexcept
{
    fireDamage_ = true;
    return *this;
}

DamageSource& DamageSource::setExplosion() noexcept
{
    explosion_ = true;
    return *this;
}

DamageSource& DamageSource::setProjectile() noexcept
{
    projectile_ = true;
    return *this;
}

DamageSource& DamageSource::setMagicDamage() noexcept
{
    magicDamage_ = true;
    return *this;
}

DamageSource& DamageSource::setDamageBypassesArmor() noexcept
{
    unblockable_ = true;
    hungerDamage_ = 0.0f;
    return *this;
}

DamageSource& DamageSource::setDamageIsAbsolute() noexcept
{
    absolute_ = true;
    hungerDamage_ = 0.0f;
    return *this;
}

DamageSource& DamageSource::setDamageAllowedInCreativeMode() noexcept
{
    creativePlayer_ = true;
    return *this;
}

DamageSource DamageSource::causeMobDamage(LivingEntity& mob)
{
    DamageSource source("mob");
    source.immediate_ = &mob;
    source.trueSource_ = &mob;
    return source;
}

DamageSource DamageSource::causePlayerDamage(LivingEntity& player)
{
    DamageSource source("player");
    source.immediate_ = &player;
    source.trueSource_ = &player;
    return source;
}

DamageSource DamageSource::causeThrownDamage(Entity& immediate, LivingEntity* owner)
{
    DamageSource source("thrown");
    source.immediate_ = &immediate;
    source.trueSource_ = owner;
    source.projectile_ = true;
    return source;
}

DamageSource DamageSource::causeIndirectMagicDamage(
    Entity& immediate,
    LivingEntity* owner)
{
    DamageSource source("indirectMagic");
    source.immediate_ = &immediate;
    source.trueSource_ = owner;
    source.magicDamage_ = true;
    source.unblockable_ = true;
    return source;
}

DamageSource DamageSource::causeThornsDamage(LivingEntity& sourceEntity)
{
    DamageSource source("thorns");
    source.immediate_ = &sourceEntity;
    source.trueSource_ = &sourceEntity;
    source.magicDamage_ = true;
    return source;
}

DamageSource DamageSource::causeExplosionDamage(LivingEntity* source)
{
    DamageSource damage("explosion.player");
    damage.trueSource_ = source;
    damage.immediate_ = source;
    damage.explosion_ = true;
    return damage;
}

DamageSource DamageSource::causeArrowDamage(Entity& arrow, LivingEntity* shooter)
{
    DamageSource source("arrow");
    source.immediate_ = &arrow;
    source.trueSource_ = shooter;
    source.projectile_ = true;
    return source;
}

const DamageSource DamageSource::IN_FIRE =
    DamageSource("inFire").setFireDamage();
const DamageSource DamageSource::LIGHTNING_BOLT = DamageSource("lightningBolt");
const DamageSource DamageSource::ON_FIRE =
    DamageSource("onFire").setFireDamage().setDamageBypassesArmor();
const DamageSource DamageSource::LAVA =
    DamageSource("lava").setFireDamage();
const DamageSource DamageSource::HOT_FLOOR =
    DamageSource("hotFloor").setFireDamage();
const DamageSource DamageSource::IN_WALL =
    DamageSource("inWall").setDamageBypassesArmor();
const DamageSource DamageSource::CRAMMING =
    DamageSource("cramming").setDamageBypassesArmor();
const DamageSource DamageSource::DROWN =
    DamageSource("drown").setDamageBypassesArmor();
const DamageSource DamageSource::STARVE =
    DamageSource("starve").setDamageBypassesArmor().setDamageIsAbsolute();
const DamageSource DamageSource::CACTUS = DamageSource("cactus");
const DamageSource DamageSource::FALL =
    DamageSource("fall").setDamageBypassesArmor();
const DamageSource DamageSource::FLY_INTO_WALL =
    DamageSource("flyIntoWall").setDamageBypassesArmor();
const DamageSource DamageSource::OUT_OF_WORLD =
    DamageSource("outOfWorld").setDamageBypassesArmor().setDamageAllowedInCreativeMode();
const DamageSource DamageSource::GENERIC =
    DamageSource("generic").setDamageBypassesArmor();
const DamageSource DamageSource::MAGIC =
    DamageSource("magic").setDamageBypassesArmor().setMagicDamage();
const DamageSource DamageSource::WITHER =
    DamageSource("wither").setDamageBypassesArmor();
const DamageSource DamageSource::ANVIL = DamageSource("anvil");
const DamageSource DamageSource::FALLING_BLOCK = DamageSource("fallingBlock");
const DamageSource DamageSource::DRAGON_BREATH =
    DamageSource("dragonBreath").setDamageBypassesArmor();
const DamageSource DamageSource::FIREWORKS =
    DamageSource("fireworks").setExplosion();
}
