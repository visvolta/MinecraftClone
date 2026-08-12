#include "entity/LivingEntity.h"

#include "BlockShape.h"
#include "World.h"
#include "entity/CombatRules.h"
#include "entity/Math.h"
#include "entity/PlayerEntity.h"
#include "entity/item/XpOrbEntity.h"

#include <algorithm>
#include <cmath>

namespace mc::entity
{
LivingEntity::LivingEntity(World& world)
    : Entity(world)
{
    applyEntityAttributes();
    health_ = getMaxHealth();
    stepHeight = 0.6f;
    rotationYaw = static_cast<float>(rand_.nextDouble() * 6.283185307179586);
    rotationYawHead = rotationYaw;
    setPosition(posX, posY, posZ);
}

void LivingEntity::applyEntityAttributes()
{
    attributes_.registerAttribute(SharedMonsterAttributes::MAX_HEALTH);
    attributes_.registerAttribute(SharedMonsterAttributes::KNOCKBACK_RESISTANCE);
    attributes_.registerAttribute(SharedMonsterAttributes::MOVEMENT_SPEED);
    attributes_.registerAttribute(SharedMonsterAttributes::ARMOR);
    attributes_.registerAttribute(SharedMonsterAttributes::ARMOR_TOUGHNESS);
}

AttributeInstance& LivingEntity::getEntityAttribute(const IAttribute& attribute)
{
    if (AttributeInstance* instance = attributes_.getAttributeInstance(attribute))
        return *instance;
    return attributes_.registerAttribute(attribute);
}

float LivingEntity::getMaxHealth()
{
    return static_cast<float>(
        getEntityAttribute(SharedMonsterAttributes::MAX_HEALTH).getAttributeValue());
}

int LivingEntity::getTotalArmorValue()
{
    return static_cast<int>(
        getEntityAttribute(SharedMonsterAttributes::ARMOR).getAttributeValue());
}

float LivingEntity::getArmorToughness()
{
    return static_cast<float>(
        getEntityAttribute(SharedMonsterAttributes::ARMOR_TOUGHNESS)
            .getAttributeValue());
}

void LivingEntity::setHealth(float health)
{
    health_ = std::clamp(health, 0.0f, getMaxHealth());
}

void LivingEntity::heal(float amount)
{
    if (amount <= 0.0f || !isAlive())
        return;
    setHealth(getHealth() + amount);
}

bool LivingEntity::isAlive() const
{
    return !dead_ && health_ > 0.0f && !isDead();
}

void LivingEntity::onEntityUpdate()
{
    Entity::onEntityUpdate();
    prevRenderYawOffset = renderYawOffset;
    prevRotationYawHead = rotationYawHead;
}

void LivingEntity::onUpdate()
{
    Entity::onUpdate();
    if (isDead())
        return;

    prevSwingProgress_ = swingProgress_;
    updateArmSwingProgress();

    if (hurtResistantTime > 0)
        --hurtResistantTime;
    if (hurtTime > 0)
        --hurtTime;
    if (recentlyHit_ > 0)
        --recentlyHit_;
    if (revengeTimer_ > 0)
    {
        --revengeTimer_;
        if (revengeTimer_ == 0)
            revengeTarget_ = nullptr;
    }

    if (!isAlive())
    {
        ++deathTime;
        if (deathTime >= 20)
            setDead();
        wrapRenderAngles();
        return;
    }

    onLivingUpdate();
    updateRenderYawOffset();
    wrapRenderAngles();
}

void LivingEntity::onLivingUpdate()
{
    if (jumpTicks_ > 0)
        --jumpTicks_;

    updatePotionEffects();

    if (std::abs(motionX) < 0.003)
        motionX = 0.0;
    if (std::abs(motionY) < 0.003)
        motionY = 0.0;
    if (std::abs(motionZ) < 0.003)
        motionZ = 0.0;

    if (!isAlive())
    {
        isJumping_ = false;
        moveStrafing = 0.0f;
        moveForward = 0.0f;
    }
    else
    {
        updateEntityActionState();
    }

    if (isJumping_)
    {
        if (isInWater())
            handleJumpWater();
        else if (isInLava())
            handleJumpLava();
        else if (onGround && jumpTicks_ == 0)
        {
            jump();
            jumpTicks_ = 10;
        }
    }
    else
    {
        jumpTicks_ = 0;
    }

    moveStrafing *= 0.98f;
    moveForward *= 0.98f;
    travel(moveStrafing, moveVertical, moveForward);
    collideWithNearbyEntities();
    updateLimbSwing();
    updateDistanceWalked();
}

void LivingEntity::updateRenderYawOffset()
{
    const double dx = posX - prevPosX;
    const double dz = posZ - prevPosZ;
    float bodyYaw = renderYawOffset;
    if (dx * dx + dz * dz > 0.0025000002)
    {
        bodyYaw = toDegrees(static_cast<float>(std::atan2(dz, dx))) - 90.0f;
        const float facingDelta = std::abs(wrapDegrees(rotationYaw) - bodyYaw);
        if (95.0f < facingDelta && facingDelta < 265.0f)
            bodyYaw -= 180.0f;
    }
    if (swingProgress_ > 0.0f)
        bodyYaw = rotationYaw;

    float f = wrapDegrees(bodyYaw - renderYawOffset);
    renderYawOffset += f * 0.3f;
    float headDelta = wrapDegrees(rotationYaw - renderYawOffset);
    headDelta = std::clamp(headDelta, -75.0f, 75.0f);
    renderYawOffset = rotationYaw - headDelta;
    if (headDelta * headDelta > 2500.0f)
        renderYawOffset += headDelta * 0.2f;
}

void LivingEntity::wrapRenderAngles()
{
    wrapAnglePair(rotationYaw, prevRotationYaw);
    wrapAnglePair(renderYawOffset, prevRenderYawOffset);
    wrapAnglePair(rotationPitch, prevRotationPitch);
    wrapAnglePair(rotationYawHead, prevRotationYawHead);
}

void LivingEntity::clearDeadEntityReferences(const Entity* removed)
{
    if (removed == nullptr)
        return;
    if (revengeTarget_ == removed)
        setRevengeTarget(nullptr);
    if (lastAttackedEntity_ == removed)
        setLastAttackedEntity(nullptr);
    if (attackingPlayer_ == removed)
        attackingPlayer_ = nullptr;
}

float LivingEntity::getBrightness() const
{
    const int x = floorInt(posX);
    const int y = floorInt(posY + getEyeHeight());
    const int z = floorInt(posZ);
    return world_->getLightBrightness(x, y, z);
}

float LivingEntity::getRenderBrightness() const
{
    const int x = floorInt(posX);
    const int y = floorInt(posY + getEyeHeight());
    const int z = floorInt(posZ);
    return world_->getRenderLightBrightness(x, y, z);
}

void LivingEntity::travel(float strafe, float vertical, float forward)
{
    if (isInWater() && !isPlayer())
    {
        const double startY = posY;
        moveRelative(strafe, vertical, forward, 0.02f);
        move(MoverType::Self, motionX, motionY, motionZ);
        motionX *= 0.800000011920929;
        motionY *= 0.800000011920929;
        motionZ *= 0.800000011920929;
        if (!isPushedByWater())
        {
            // squids etc.
        }
        else
        {
            motionY -= 0.02;
        }
        if (collidedHorizontally &&
            isOffsetPositionInLiquid(
                motionX, motionY + 0.6000000238418579 - posY + startY, motionZ))
        {
            motionY = 0.30000001192092896;
        }
        return;
    }

    if (isInLava() && !isPlayer())
    {
        const double startY = posY;
        moveRelative(strafe, vertical, forward, 0.02f);
        move(MoverType::Self, motionX, motionY, motionZ);
        motionX *= 0.5;
        motionY *= 0.5;
        motionZ *= 0.5;
        if (!isPushedByWater())
        {
        }
        else
        {
            motionY -= 0.02;
        }
        if (collidedHorizontally &&
            isOffsetPositionInLiquid(
                motionX, motionY + 0.6000000238418579 - posY + startY, motionZ))
        {
            motionY = 0.30000001192092896;
        }
        return;
    }

    float friction = 0.91f;
    if (onGround)
    {
        const int bx = floorInt(posX);
        const int by = floorInt(boundingBox_.minY) - 1;
        const int bz = floorInt(posZ);
        const BlockType ground = world_->getBlock(bx, by, bz);
        float slipperiness = 0.6f;
        if (ground == BlockType::Ice)
            slipperiness = 0.98f;
        friction = slipperiness * 0.91f;
    }

    const float frictionCubed = friction * friction * friction;
    const float accel = onGround
        ? getAIMoveSpeed() * (0.16277136f / frictionCubed)
        : jumpMovementFactor;
    moveRelative(strafe, vertical, forward, accel);

    friction = 0.91f;
    if (onGround)
    {
        const int bx = floorInt(posX);
        const int by = floorInt(boundingBox_.minY) - 1;
        const int bz = floorInt(posZ);
        const BlockType ground = world_->getBlock(bx, by, bz);
        float slipperiness = 0.6f;
        if (ground == BlockType::Ice)
            slipperiness = 0.98f;
        friction = slipperiness * 0.91f;
    }

    if (isOnLadder())
    {
        motionX = std::clamp(motionX, -0.15000000596046448, 0.15000000596046448);
        motionZ = std::clamp(motionZ, -0.15000000596046448, 0.15000000596046448);
        fallDistance = 0.0f;
        if (motionY < -0.15)
            motionY = -0.15;
        if (isSneaking() && isPlayer() && motionY < 0.0)
            motionY = 0.0;
    }

    move(MoverType::Self, motionX, motionY, motionZ);

    if (collidedHorizontally && isOnLadder())
        motionY = 0.2;

    if (isPotionActive(gameplay::StatusEffectType::Levitation))
    {
        motionY += (0.05 * static_cast<double>(
            getPotionAmplifier(gameplay::StatusEffectType::Levitation) + 1) -
            motionY) * 0.2;
    }
    else if (!isInWater())
    {
        motionY -= 0.08;
    }

    motionY *= 0.9800000190734863;
    motionX *= static_cast<double>(friction);
    motionZ *= static_cast<double>(friction);
}

bool LivingEntity::isOnLadder() const
{
    const int x = floorInt(posX);
    const int y = floorInt(boundingBox_.minY);
    const int z = floorInt(posZ);
    const BlockType block = world_->getBlock(x, y, z);
    return isLadder(block) || block == BlockType::Vine;
}

void LivingEntity::jump()
{
    motionY = static_cast<double>(getJumpUpwardsMotion());
    if (isPotionActive(gameplay::StatusEffectType::JumpBoost))
    {
        motionY += 0.1 * static_cast<double>(
            getPotionAmplifier(gameplay::StatusEffectType::JumpBoost) + 1);
    }
    if (isSprinting())
    {
        const float yaw = toRadians(rotationYaw);
        motionX -= static_cast<double>(std::sin(yaw) * 0.2f);
        motionZ += static_cast<double>(std::cos(yaw) * 0.2f);
    }
    isAirBorne = true;
}

void LivingEntity::handleJumpWater()
{
    motionY += 0.03999999910593033;
}

void LivingEntity::handleJumpLava()
{
    motionY += 0.03999999910593033;
}

bool LivingEntity::attackEntityFrom(const DamageSource& source, float amount)
{
    if (isDead() || health_ <= 0.0f)
        return false;
    if (source.isFireDamage() &&
        isPotionActive(gameplay::StatusEffectType::FireResistance))
        return false;

    idleTime_ = 0;

    if (amount > 0.0f && canBlockDamageSource(source))
    {
        amount = 0.0f;
        if (!source.isProjectile())
        {
            if (LivingEntity* attacker =
                    dynamic_cast<LivingEntity*>(source.getImmediateSource()))
            {
                const double dx = attacker->posX - posX;
                const double dz = attacker->posZ - posZ;
                attacker->knockBack(this, 0.5f, dx, dz);
            }
        }
        limbSwingAmount = 1.5f;
        return false;
    }

    limbSwingAmount = 1.5f;
    bool firstHit = true;
    if (static_cast<float>(hurtResistantTime) >
        static_cast<float>(maxHurtResistantTime) / 2.0f)
    {
        if (amount <= lastDamage_)
            return false;
        damageEntity(source, amount - lastDamage_);
        lastDamage_ = amount;
        firstHit = false;
    }
    else
    {
        lastDamage_ = amount;
        hurtResistantTime = maxHurtResistantTime;
        damageEntity(source, amount);
        maxHurtTime = 10;
        hurtTime = maxHurtTime;
    }

    attackedAtYaw = 0.0f;
    if (LivingEntity* attacker = source.getTrueSource())
    {
        setRevengeTarget(attacker);
        if (attacker->isPlayer())
        {
            recentlyHit_ = 100;
            attackingPlayer_ = static_cast<PlayerEntity*>(attacker);
        }
        const double dx = attacker->posX - posX;
        const double dz = attacker->posZ - posZ;
        if (dx * dx + dz * dz > 1.0e-8)
            attackedAtYaw = toDegrees(static_cast<float>(
                std::atan2(dz, dx))) - rotationYaw;
        if (firstHit && amount > 0.0f)
            knockBack(attacker, 0.4f, dx, dz);
    }
    else if (firstHit)
    {
        attackedAtYaw = static_cast<float>((ticksExisted_ & 1) ? 180.0 : 0.0);
    }

    if (health_ <= 0.0f)
        onDeath(source);
    return true;
}

void LivingEntity::damageEntity(const DamageSource& source, float amount)
{
    amount = applyArmorCalculations(source, amount);
    amount = applyPotionDamageCalculations(source, amount);
    amount = std::max(0.0f, amount);
    if (amount == 0.0f)
        return;
    setHealth(health_ - amount);
}

float LivingEntity::applyArmorCalculations(
    const DamageSource& source,
    float damage)
{
    if (!source.isUnblockable())
    {
        damageArmor(damage);
        damage = CombatRules::getDamageAfterAbsorb(
            damage,
            static_cast<float>(getTotalArmorValue()),
            getArmorToughness()
        );
    }
    return damage;
}

float LivingEntity::applyPotionDamageCalculations(
    const DamageSource& source,
    float damage)
{
    if (source.isDamageAbsolute())
        return damage;
    if (isPotionActive(gameplay::StatusEffectType::Resistance) &&
        source.getDamageType() != std::string_view("outOfWorld"))
    {
        const int amp =
            (getPotionAmplifier(gameplay::StatusEffectType::Resistance) + 1) * 5;
        damage = damage * static_cast<float>(25 - amp) / 25.0f;
    }
    return std::max(0.0f, damage);
}

bool LivingEntity::canBlockDamageSource(const DamageSource& source) const
{
    if (source.isUnblockable())
        return false;
    return false;
}

void LivingEntity::knockBack(
    Entity*,
    float strength,
    double xRatio,
    double zRatio)
{
    if (rand_.nextDouble() >=
        getEntityAttribute(SharedMonsterAttributes::KNOCKBACK_RESISTANCE)
            .getAttributeValue())
    {
        isAirBorne = true;
        const float length = static_cast<float>(
            std::sqrt(xRatio * xRatio + zRatio * zRatio));
        if (length < 1.0e-6f)
            return;
        motionX /= 2.0;
        motionZ /= 2.0;
        motionX -= xRatio / static_cast<double>(length) * static_cast<double>(strength);
        motionZ -= zRatio / static_cast<double>(length) * static_cast<double>(strength);
        if (onGround)
        {
            motionY /= 2.0;
            motionY += static_cast<double>(strength);
            if (motionY > 0.4000000059604645)
                motionY = 0.4000000059604645;
        }
    }
}

void LivingEntity::fall(float distance, float damageMultiplier)
{
    Entity::fall(distance, damageMultiplier);
    float jumpBoost = 0.0f;
    if (isPotionActive(gameplay::StatusEffectType::JumpBoost))
        jumpBoost = static_cast<float>(
            getPotionAmplifier(gameplay::StatusEffectType::JumpBoost) + 1);
    const int damage = ceilInt((distance - 3.0f - jumpBoost) * damageMultiplier);
    if (damage > 0)
        attackEntityFrom(DamageSource::FALL, static_cast<float>(damage));
}

void LivingEntity::onDeath(const DamageSource& source)
{
    if (dead_)
        return;
    dead_ = true;
    if (PlayerEntity* player = attackingPlayer_)
    {
        const int xp = getExperiencePoints(player);
        if (xp > 0)
            world_->spawnXpOrbs(posX, posY, posZ, xp);
    }
    dropLoot(recentlyHit_ > 0, 0, source);
}

void LivingEntity::dropLoot(bool wasRecentlyHit, int, const DamageSource&)
{
    const core::ResourceLocation table = getLootTable();
    if (table.path() == "empty" || table.path().empty())
        return;
    // Resource pack loot is rolled by the world helper.
    world_->dropLootTable(
        table,
        posX,
        posY,
        posZ,
        wasRecentlyHit
    );
}

int LivingEntity::getExperiencePoints(PlayerEntity*) const
{
    return experienceValue_;
}

core::ResourceLocation LivingEntity::getLootTable() const
{
    return core::ResourceLocation("minecraft:entities/empty");
}

void LivingEntity::setRevengeTarget(LivingEntity* target)
{
    revengeTarget_ = target;
    revengeTimer_ = revengeTarget_ ? 100 : 0;
}

void LivingEntity::setLastAttackedEntity(LivingEntity* entity)
{
    lastAttackedEntity_ = entity;
    lastAttackedEntityTime_ = ticksExisted_;
}

void LivingEntity::addPotionEffect(const gameplay::StatusEffect& effect)
{
    effects_.addEffect(effect);
}

bool LivingEntity::isPotionActive(gameplay::StatusEffectType type) const
{
    return effects_.hasEffect(type);
}

int LivingEntity::getPotionAmplifier(gameplay::StatusEffectType type) const
{
    return effects_.effectAmplifier(type);
}

void LivingEntity::clearActivePotions()
{
    effects_.clearEffects();
}

void LivingEntity::updatePotionEffects()
{
    int dummyHealth = static_cast<int>(health_);
    effects_.tick(true, false, dummyHealth, static_cast<int>(getMaxHealth()));
    if (static_cast<float>(dummyHealth) < health_)
        heal(static_cast<float>(dummyHealth) - health_ + 0.0f);
}

void LivingEntity::swingArm()
{
    if (!swingInProgress_ ||
        swingProgressInt_ >= getArmSwingAnimationEnd() / 2 ||
        swingProgressInt_ < 0)
    {
        swingProgressInt_ = -1;
        swingInProgress_ = true;
    }
}

void LivingEntity::updateArmSwingProgress()
{
    const int end = getArmSwingAnimationEnd();
    if (swingInProgress_)
    {
        ++swingProgressInt_;
        if (swingProgressInt_ >= end)
        {
            swingProgressInt_ = 0;
            swingInProgress_ = false;
        }
    }
    else
    {
        swingProgressInt_ = 0;
    }
    swingProgress_ = static_cast<float>(swingProgressInt_) / static_cast<float>(end);
}

float LivingEntity::getSwingProgress(float partialTick) const
{
    float progress = swingProgress_ - prevSwingProgress_;
    if (progress < 0.0f)
        progress += 1.0f;
    return prevSwingProgress_ + progress * partialTick;
}

void LivingEntity::updateLimbSwing()
{
    prevLimbSwingAmount = limbSwingAmount;
    const double dx = posX - prevPosX;
    const double dz = posZ - prevPosZ;
    float distance = static_cast<float>(std::sqrt(dx * dx + dz * dz)) * 4.0f;
    if (distance > 1.0f)
        distance = 1.0f;
    limbSwingAmount += (distance - limbSwingAmount) * 0.4f;
    limbSwing += limbSwingAmount;
}

void LivingEntity::updateDistanceWalked()
{
}

void LivingEntity::collideWithNearbyEntities()
{
    const AxisAlignedBB box = boundingBox_.grow(0.2, 0.0, 0.2);
    for (Entity* other : world_->getEntitiesInAABB(box, this))
    {
        if (other == this || !other->canBePushed())
            continue;
        collideWithEntity(*other);
    }
}

void LivingEntity::collideWithEntity(Entity& entity)
{
    entity.applyEntityCollision(*this);
}

void LivingEntity::dripOutOfWater()
{
}
}

// Silence unused PackedIce/FrostedIce if those block types don't exist.
// LivingEntity.cpp referenced BlockType::PackedIce, FrostedIce, Slime, Vine.
// Check Block.h - we have Ice, Snow, Vine. No PackedIce/FrostedIce/Slime as BlockType.
// I need to fix those references.
