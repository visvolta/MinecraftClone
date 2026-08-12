#pragma once

#include "entity/AxisAlignedBB.h"
#include "entity/DamageSource.h"
#include "entity/EntityUuid.h"
#include "core/ResourceLocation.h"
#include "worldgen/JavaRandom.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <string>
#include <vector>

class World;

namespace mc::entity
{
class LivingEntity;
class PlayerEntity;

enum class MoverType
{
    Self,
    Player,
    Piston,
    ShulkerBox,
    Shulker
};

enum class EntityKind
{
    Generic,
    Living,
    Player,
    Item,
    XpOrb,
    Projectile
};

class Entity
{
public:
    explicit Entity(World& world);
    virtual ~Entity() = default;

    Entity(const Entity&) = delete;
    Entity& operator=(const Entity&) = delete;

    virtual void onUpdate();
    virtual void onEntityUpdate();
    virtual bool attackEntityFrom(const DamageSource& source, float amount);
    virtual void onKillEntity(LivingEntity&) {}
    virtual void applyEntityCollision(Entity& other);
    virtual void onCollideWithPlayer(PlayerEntity&) {}
    virtual void fall(float distance, float damageMultiplier);
    virtual float getEyeHeight() const;
    virtual bool canBeCollidedWith() const;
    virtual bool canBePushed() const;
    virtual bool isPushedByWater() const { return true; }
    virtual bool isImmuneToExplosions() const { return false; }
    [[nodiscard]] virtual core::ResourceLocation getType() const;
    [[nodiscard]] virtual EntityKind entityKind() const noexcept
    {
        return EntityKind::Generic;
    }
    [[nodiscard]] virtual bool isInRangeToRender3d(
        double x, double y, double z) const;

    void setSize(float width, float height);
    void setPosition(double x, double y, double z);
    void setLocationAndAngles(
        double x, double y, double z, float yaw, float pitch);
    void setPositionAndUpdate(double x, double y, double z);
    void move(MoverType type, double x, double y, double z);
    void moveRelative(float strafe, float up, float forward, float friction);
    void addVelocity(double x, double y, double z);
    void setDead();

    [[nodiscard]] bool isDead() const noexcept { return isDead_; }
    [[nodiscard]] bool isAlive() const;
    [[nodiscard]] bool isBurning() const noexcept { return fire_ > 0; }
    [[nodiscard]] bool isInWater() const noexcept { return inWater_; }
    [[nodiscard]] bool isInLava() const noexcept;
    [[nodiscard]] bool isImmuneToFire() const noexcept { return immuneToFire_; }
    [[nodiscard]] bool isSneaking() const noexcept { return sneaking_; }
    [[nodiscard]] bool isSprinting() const noexcept { return sprinting_; }
    [[nodiscard]] bool isRiding() const noexcept { return ridingEntity_ != nullptr; }
    [[nodiscard]] bool isBeingRidden() const noexcept { return !riddenBy_.empty(); }
    [[nodiscard]] Entity* getRidingEntity() const noexcept { return ridingEntity_; }
    [[nodiscard]] Entity* getControllingPassenger() const noexcept;
    void startRiding(Entity& vehicle);
    void dismountRidingEntity();
    void removePassengers();
    void updatePassenger(Entity& passenger);

    bool handleWaterMovement();
    void extinguish() noexcept { fire_ = 0; }
    void setFire(int seconds);
    void dealFireDamage(int amount);
    void setOnFireFromLava();

    [[nodiscard]] World& getWorld() noexcept { return *world_; }
    [[nodiscard]] const World& getWorld() const noexcept { return *world_; }
    [[nodiscard]] const AxisAlignedBB& getEntityBoundingBox() const noexcept
    {
        return boundingBox_;
    }
    void setEntityBoundingBox(const AxisAlignedBB& box) noexcept
    {
        boundingBox_ = box;
    }
    void resetPositionToBB();

    [[nodiscard]] glm::vec3 getPositionVec() const noexcept;
    [[nodiscard]] glm::vec3 getInterpolatedPosition(float partialTick) const;
    [[nodiscard]] glm::vec3 getLookVec() const;
    [[nodiscard]] double getDistanceSq(const Entity& other) const;
    [[nodiscard]] double getDistanceSq(double x, double y, double z) const;
    [[nodiscard]] float getDistance(const Entity& other) const;
    [[nodiscard]] bool canEntityBeSeen(const Entity& other) const;

    [[nodiscard]] const EntityUuid& uuid() const noexcept { return uuid_; }
    void setUuid(const EntityUuid& uuid) noexcept { uuid_ = uuid; }
    [[nodiscard]] JavaRandom& getRNG() noexcept { return rand_; }

    [[nodiscard]] float getWidth() const noexcept { return width_; }
    [[nodiscard]] float getHeight() const noexcept { return height_; }
    [[nodiscard]] int getTicksExisted() const noexcept { return ticksExisted_; }

    double posX = 0.0;
    double posY = 0.0;
    double posZ = 0.0;
    double prevPosX = 0.0;
    double prevPosY = 0.0;
    double prevPosZ = 0.0;
    double lastTickPosX = 0.0;
    double lastTickPosY = 0.0;
    double lastTickPosZ = 0.0;
    double motionX = 0.0;
    double motionY = 0.0;
    double motionZ = 0.0;
    float rotationYaw = 0.0f;
    float rotationPitch = 0.0f;
    float prevRotationYaw = 0.0f;
    float prevRotationPitch = 0.0f;
    bool onGround = false;
    bool collidedHorizontally = false;
    bool collidedVertically = false;
    bool collided = false;
    bool velocityChanged = false;
    bool isAirBorne = false;
    float width_ = 0.6f;
    float height_ = 1.8f;
    float stepHeight = 0.0f;
    float fallDistance = 0.0f;
    float entityCollisionReduction = 0.0f;
    bool noClip = false;
    int ticksExisted_ = 0;
    int fire_ = 0;
    int hurtResistantTime = 0;

protected:
    World* world_ = nullptr;
    JavaRandom rand_;
    EntityUuid uuid_ = EntityUuid::random();
    AxisAlignedBB boundingBox_{};
    bool isDead_ = false;
    bool inWater_ = false;
    bool firstUpdate_ = true;
    bool immuneToFire_ = false;
    bool sneaking_ = false;
    bool sprinting_ = false;
    bool inWeb_ = false;
    Entity* ridingEntity_ = nullptr;
    std::vector<Entity*> riddenBy_;
    int rideCooldown_ = 0;

    virtual void doBlockCollisions();
    virtual void updateFallState(
        double y,
        bool onGroundIn,
        int blockX,
        int blockY,
        int blockZ
    );
    [[nodiscard]] virtual bool isOffsetPositionInLiquid(
        double x,
        double y,
        double z
    ) const;
    void setFlagSneaking(bool value) noexcept { sneaking_ = value; }
    void setFlagSprinting(bool value) noexcept { sprinting_ = value; }

private:
    void collectCollisionBoxes(
        const AxisAlignedBB& area,
        std::vector<AxisAlignedBB>& out
    ) const;
};
}
