#pragma once

#include "Item.h"
#include "entity/Entity.h"
#include "entity/attributes/AttributeMap.h"
#include "entity/attributes/SharedMonsterAttributes.h"
#include "gameplay/SurvivalStats.h"

#include <array>
#include <cstdint>
#include <optional>

namespace mc::entity
{
class LivingEntity : public Entity
{
public:
    explicit LivingEntity(World& world);

    void onUpdate() override;
    virtual void onLivingUpdate();
    void travel(float strafe, float vertical, float forward);
    bool attackEntityFrom(const DamageSource& source, float amount) override;
    void knockBack(Entity* attacker, float strength, double xRatio, double zRatio);
    void fall(float distance, float damageMultiplier) override;
    [[nodiscard]] EntityKind entityKind() const noexcept override
    {
        return EntityKind::Living;
    }
    [[nodiscard]] bool canBePushed() const override { return isAlive() && !isDead(); }
    [[nodiscard]] bool isAlive() const;

    void setHealth(float health);
    [[nodiscard]] float getHealth() const noexcept { return health_; }
    [[nodiscard]] float getMaxHealth();
    [[nodiscard]] int getTotalArmorValue();
    [[nodiscard]] float getArmorToughness();
    void heal(float amount);

    AttributeMap& getAttributeMap() noexcept { return attributes_; }
    AttributeInstance& getEntityAttribute(const IAttribute& attribute);
    void applyEntityAttributes();

    void setRevengeTarget(LivingEntity* target);
    [[nodiscard]] LivingEntity* getRevengeTarget() const noexcept
    {
        return revengeTarget_;
    }
    [[nodiscard]] LivingEntity* getLastAttackedEntity() const noexcept
    {
        return lastAttackedEntity_;
    }
    void setLastAttackedEntity(LivingEntity* entity);
    [[nodiscard]] PlayerEntity* getAttackingPlayer() const noexcept
    {
        return attackingPlayer_;
    }
    [[nodiscard]] int getRecentlyHit() const noexcept { return recentlyHit_; }

    void setJumping(bool jumping) noexcept { isJumping_ = jumping; }
    [[nodiscard]] bool isJumping() const noexcept { return isJumping_; }
    void setMoveForward(float value) noexcept { moveForward = value; }
    void setMoveStrafing(float value) noexcept { moveStrafing = value; }
    void setAIMoveSpeed(float speed) noexcept { landMovementFactor_ = speed; }
    [[nodiscard]] float getAIMoveSpeed() const noexcept { return landMovementFactor_; }

    [[nodiscard]] bool isOnLadder() const;
    [[nodiscard]] virtual bool isChild() const { return false; }
    [[nodiscard]] virtual bool isPlayer() const noexcept { return false; }
    [[nodiscard]] bool canBreatheUnderwater() const noexcept
    {
        return canBreatheUnderwater_;
    }
    [[nodiscard]] bool isEntityUndead() const noexcept { return undead_; }

    void addPotionEffect(const gameplay::StatusEffect& effect);
    [[nodiscard]] bool isPotionActive(gameplay::StatusEffectType type) const;
    [[nodiscard]] int getPotionAmplifier(gameplay::StatusEffectType type) const;
    void clearActivePotions();

    [[nodiscard]] ItemStack& getHeldItemMainhand() noexcept { return hands_[0]; }
    [[nodiscard]] const ItemStack& getHeldItemMainhand() const noexcept
    {
        return hands_[0];
    }
    [[nodiscard]] ItemStack& getHeldItemOffhand() noexcept { return hands_[1]; }
    [[nodiscard]] std::array<ItemStack, 4>& armorInventory() noexcept
    {
        return armor_;
    }

    [[nodiscard]] float getSwingProgress(float partialTick) const;
    void swingArm();

    float renderYawOffset = 0.0f;
    float prevRenderYawOffset = 0.0f;
    float rotationYawHead = 0.0f;
    float prevRotationYawHead = 0.0f;
    float limbSwing = 0.0f;
    float limbSwingAmount = 0.0f;
    float prevLimbSwingAmount = 0.0f;
    float moveStrafing = 0.0f;
    float moveVertical = 0.0f;
    float moveForward = 0.0f;
    float jumpMovementFactor = 0.02f;
    int hurtTime = 0;
    int maxHurtTime = 10;
    int deathTime = 0;
    int maxHurtResistantTime = 20;
    float attackedAtYaw = 0.0f;

    [[nodiscard]] virtual int getExperiencePoints(PlayerEntity* player) const;
    [[nodiscard]] virtual core::ResourceLocation getLootTable() const;

protected:
    virtual void damageEntity(const DamageSource& source, float amount);
    virtual float applyArmorCalculations(const DamageSource& source, float damage);
    virtual float applyPotionDamageCalculations(
        const DamageSource& source,
        float damage
    );
    virtual void damageArmor(float) {}
    virtual bool canBlockDamageSource(const DamageSource& source) const;
    virtual void dropLoot(bool wasRecentlyHit, int looting, const DamageSource& source);
    virtual void onDeath(const DamageSource& source);
    virtual void jump();
    virtual void handleJumpWater();
    virtual void handleJumpLava();
    [[nodiscard]] virtual float getJumpUpwardsMotion() const noexcept { return 0.42f; }
    virtual void updateArmSwingProgress();
    virtual void updateEntityActionState() {}
    virtual void collideWithNearbyEntities();
    virtual void collideWithEntity(Entity& entity);
    [[nodiscard]] virtual int getArmSwingAnimationEnd() const noexcept { return 6; }
    void updateDistanceWalked();
    void dripOutOfWater();

    AttributeMap attributes_;
    gameplay::SurvivalStats effects_;
    float health_ = 20.0f;
    float lastDamage_ = 0.0f;
    int idleTime_ = 0;
    int recentlyHit_ = 0;
    int revengeTimer_ = 0;
    int lastAttackedEntityTime_ = 0;
    int air_ = 300;
    int jumpTicks_ = 0;
    bool isJumping_ = false;
    bool dead_ = false;
    bool canBreatheUnderwater_ = false;
    bool undead_ = false;
    int experienceValue_ = 0;
    LivingEntity* revengeTarget_ = nullptr;
    LivingEntity* lastAttackedEntity_ = nullptr;
    PlayerEntity* attackingPlayer_ = nullptr;
    std::array<ItemStack, 2> hands_{};
    std::array<ItemStack, 4> armor_{};
    bool swingInProgress_ = false;
    int swingProgressInt_ = 0;
    float swingProgress_ = 0.0f;
    float prevSwingProgress_ = 0.0f;
    float landMovementFactor_ = 0.0f;
    int scoreValue_ = 0;

    void updatePotionEffects();
    void updateLimbSwing();
    void setRotationYawHead(float yaw) noexcept { rotationYawHead = yaw; }
};
}
