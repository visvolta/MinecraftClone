#include "entity/PlayerEntity.h"

#include "BlockShape.h"
#include "Camera.h"
#include "World.h"
#include "entity/Math.h"

#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>

namespace mc::entity
{
PlayerEntity::PlayerEntity(World& world, glm::vec3 feetPosition)
    : LivingEntity(world)
{
    setSize(0.6f, 1.8f);
    setPosition(feetPosition.x, feetPosition.y, feetPosition.z);
    cachedPosition_ = feetPosition;
    renderPosition_ = feetPosition;
    prevPosX = posX;
    prevPosY = posY;
    prevPosZ = posZ;
    getEntityAttribute(SharedMonsterAttributes::MAX_HEALTH).setBaseValue(20.0);
    getEntityAttribute(SharedMonsterAttributes::MOVEMENT_SPEED).setBaseValue(0.1);
    getEntityAttribute(SharedMonsterAttributes::ATTACK_SPEED).setBaseValue(4.0);
    setHealth(20.0f);
    experienceValue_ = 0;
}

core::ResourceLocation PlayerEntity::getType() const
{
    return core::ResourceLocation("minecraft:player");
}

float PlayerEntity::getEyeHeight() const
{
    return isSneaking() ? 1.54f : 1.62f;
}

const glm::vec3& PlayerEntity::getPosition() const
{
    const_cast<PlayerEntity*>(this)->cachedPosition_ = {
        static_cast<float>(posX),
        static_cast<float>(posY),
        static_cast<float>(posZ)
    };
    return cachedPosition_;
}

glm::vec3 PlayerEntity::getEyePosition() const
{
    return getPosition() + glm::vec3(0.0f, getEyeHeight(), 0.0f);
}

glm::vec3 PlayerEntity::getRenderPosition() const
{
    return renderPosition_;
}

glm::vec3 PlayerEntity::getRenderEyePosition() const
{
    return renderPosition_ + glm::vec3(0.0f, getEyeHeight(), 0.0f);
}

const glm::vec3& PlayerEntity::getLookDirection() const noexcept
{
    return lookDirection_;
}

bool PlayerEntity::overlapsBlock(
    const glm::ivec3& blockPosition,
    BlockType block) const
{
    const BlockBox playerBox{
        {static_cast<float>(boundingBox_.minX),
         static_cast<float>(boundingBox_.minY),
         static_cast<float>(boundingBox_.minZ)},
        {static_cast<float>(boundingBox_.maxX),
         static_cast<float>(boundingBox_.maxY),
         static_cast<float>(boundingBox_.maxZ)}
    };
    for (const BlockBox& localBox : getBlockShape(block).collisionBoxes)
        if (boxesIntersect(playerBox, translateBlockBox(localBox, blockPosition)))
            return true;
    return false;
}

int PlayerEntity::getHealth() const noexcept
{
    return static_cast<int>(std::ceil(health_));
}

int PlayerEntity::getMaximumHealth() const noexcept
{
    return static_cast<int>(std::ceil(
        const_cast<PlayerEntity*>(this)->getMaxHealth()));
}

void PlayerEntity::eat(int food, float saturationModifier) noexcept
{
    survival_.eat(food, saturationModifier);
}

void PlayerEntity::addExperience(int amount) noexcept
{
    survival_.addExperience(amount);
}

void PlayerEntity::resetAttackCooldown() noexcept
{
    survival_.resetAttackCooldown();
}

void PlayerEntity::setBlocking(bool blocking) noexcept
{
    blocking_ = blocking && isAlive();
}

void PlayerEntity::setNoClip(bool enabled) noexcept
{
    noClip = enabled;
    motionX = motionY = motionZ = 0.0;
}

void PlayerEntity::respawn(const glm::vec3& feetPosition) noexcept
{
    isDead_ = false;
    dead_ = false;
    setHealth(getMaxHealth());
    previousHealth_ = getHealth();
    air_ = 300;
    fire_ = 0;
    deathTime = 0;
    hurtTime = 0;
    hurtResistantTime = 0;
    fallDistance = 0.0f;
    survival_.respawn();
    setPosition(feetPosition.x, feetPosition.y, feetPosition.z);
    motionX = motionY = motionZ = 0.0;
    renderPosition_ = feetPosition;
}

void PlayerEntity::setRidingPosition(const glm::vec3& feetPosition) noexcept
{
    setPosition(feetPosition.x, feetPosition.y, feetPosition.z);
}

PlayerEntity::PersistentState PlayerEntity::persistentState() const noexcept
{
    return {
        getPosition(), getHealth(), previousHealth_, air_, fire_,
        ticksExisted_, uuid_, survival_.persistentState()
    };
}

void PlayerEntity::restorePersistentState(const PersistentState& state) noexcept
{
    setPosition(state.position.x, state.position.y, state.position.z);
    setHealth(static_cast<float>(state.health));
    previousHealth_ = state.previousHealth;
    air_ = state.air;
    fire_ = state.fireTicks;
    ticksExisted_ = state.ticksExisted;
    if (!state.uuid.empty())
        uuid_ = state.uuid;
    survival_.restorePersistentState(state.survival);
    renderPosition_ = state.position;
}

bool PlayerEntity::canBlockDamageSource(const DamageSource& source) const
{
    if (!blocking_ || source.isUnblockable())
        return false;
    if (Entity* attacker = source.getImmediateSource())
    {
        glm::vec3 toward{
            static_cast<float>(attacker->posX - posX),
            0.0f,
            static_cast<float>(attacker->posZ - posZ)
        };
        if (glm::dot(toward, toward) < 0.0001f)
            return false;
        toward = glm::normalize(toward);
        glm::vec3 facing = lookDirection_;
        facing.y = 0.0f;
        if (glm::dot(facing, facing) < 0.0001f)
            return false;
        return glm::dot(glm::normalize(facing), toward) > 0.0f;
    }
    return false;
}

bool PlayerEntity::attackEntityFrom(const DamageSource& source, float amount)
{
    previousHealth_ = getHealth();
    const bool hit = LivingEntity::attackEntityFrom(source, amount);
    if (hit)
        survival_.addExhaustion(0.1f);
    return hit;
}

void PlayerEntity::damage(int amount, const glm::vec3& sourcePosition) noexcept
{
    DamageSource src = DamageSource::GENERIC;
    attackEntityFrom(src, static_cast<float>(amount));
    (void)sourcePosition;
}

void PlayerEntity::update(GLFWwindow* window, float deltaTime, Camera& camera)
{
    if (noClip)
    {
        glm::vec3 direction(0.0f);
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            direction += camera.getForward();
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            direction -= camera.getForward();
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            direction += camera.getRight();
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            direction -= camera.getRight();
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            direction.y += 1.0f;
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            direction.y -= 1.0f;
        if (glm::dot(direction, direction) > 0.000001f)
            direction = glm::normalize(direction);
        posX += direction.x * 12.0f * deltaTime;
        posY += direction.y * 12.0f * deltaTime;
        posZ += direction.z * 12.0f * deltaTime;
        setPosition(posX, posY, posZ);
        renderPosition_ = getPosition();
        camera.setPosition(getRenderEyePosition());
        return;
    }

    deltaTime = std::clamp(deltaTime, 0.0f, 0.25f);
    const bool jumpPressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
    if (jumpPressed && !jumpWasPressed_)
        jumpQueued_ = true;
    jumpWasPressed_ = jumpPressed;

    physicsAccumulator_ += deltaTime;
    int steps = 0;
    while (physicsAccumulator_ >= 0.05f && steps < 5)
    {
        simulateInput(window, camera);
        onUpdate();
        physicsAccumulator_ -= 0.05f;
        ++steps;
    }
    if (steps == 5)
        physicsAccumulator_ = 0.0f;

    const float alpha = std::clamp(physicsAccumulator_ / 0.05f, 0.0f, 1.0f);
    renderPosition_ = {
        static_cast<float>(prevPosX + (posX - prevPosX) * alpha),
        static_cast<float>(prevPosY + (posY - prevPosY) * alpha),
        static_cast<float>(prevPosZ + (posZ - prevPosZ) * alpha)
    };
    camera.setPosition(getRenderEyePosition());
}

void PlayerEntity::simulateInput(GLFWwindow* window, Camera& camera)
{
    lookDirection_ = camera.getForward();
    rotationYaw = toDegrees(std::atan2(-lookDirection_.x, lookDirection_.z));
    rotationPitch = toDegrees(-std::asin(std::clamp(lookDirection_.y, -1.0f, 1.0f)));
    rotationYawHead = rotationYaw;

    if (isRiding())
    {
        setFlagSneaking(isAlive() &&
            glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS);
        setFlagSprinting(false);
        int hp = getHealth();
        survival_.tick(true, true, hp, getMaximumHealth());
        if (hp < getHealth())
            setHealth(static_cast<float>(hp));
        updateFovMultiplier();
        return;
    }

    float forward = 0.0f;
    float strafe = 0.0f;
    if (isAlive())
    {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) forward += 1.0f;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) forward -= 1.0f;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) strafe -= 1.0f;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) strafe += 1.0f;
    }

    const bool crouchKey = isAlive() &&
        glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
    setFlagSneaking(crouchKey);
    if (isSneaking())
    {
        forward *= 0.3f;
        strafe *= 0.3f;
    }

    const bool forwardKeyDown = forward >= 0.8f;
    const bool canSprint = isAlive() && !isSneaking() && survival_.foodLevel() > 6;
    const bool sprintKey = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS;
    if (!isSprinting() && canSprint && forwardKeyDown && sprintKey)
        setFlagSprinting(true);
    else if (!isSprinting() && canSprint && onGround && forwardKeyDown &&
             !forwardKeyWasDown_)
    {
        if (sprintToggleTicks_ > 0)
        {
            setFlagSprinting(true);
            sprintToggleTicks_ = 0;
        }
        else
            sprintToggleTicks_ = 7;
    }
    if (isSprinting() && (!canSprint || !forwardKeyDown || collidedHorizontally))
        setFlagSprinting(false);
    forwardKeyWasDown_ = forwardKeyDown;
    if (sprintToggleTicks_ > 0)
        --sprintToggleTicks_;

    moveForward = forward;
    moveStrafing = strafe;
    setJumping(jumpQueued_ && isAlive());
    jumpQueued_ = false;

    if (isSprinting())
        setAIMoveSpeed(0.13f);
    else if (isSneaking())
        setAIMoveSpeed(0.03f);
    else
        setAIMoveSpeed(0.1f);

    int hp = getHealth();
    survival_.tick(true, true, hp, getMaximumHealth());
    if (hp != getHealth())
        setHealth(static_cast<float>(hp));
    updateEnvironment();
    updateFovMultiplier();
}

void PlayerEntity::updateEntityActionState()
{
    // Input already filled moveForward/moveStrafing.
}

void PlayerEntity::updateEnvironment()
{
    const int x = floorInt(posX);
    const int y = floorInt(posY + getEyeHeight());
    const int z = floorInt(posZ);
    headUnderwater_ = world_->getBlock(x, y, z) == BlockType::Water;
}

void PlayerEntity::updateFovMultiplier() noexcept
{
    float movementModifier = isSprinting() ? 1.3f : 1.0f;
    const int speed = survival_.effectAmplifier(gameplay::StatusEffectType::Speed);
    const int slowness = survival_.effectAmplifier(gameplay::StatusEffectType::Slowness);
    if (speed >= 0)
        movementModifier *= 1.0f + 0.2f * static_cast<float>(speed + 1);
    if (slowness >= 0)
        movementModifier *= std::max(
            0.0f, 1.0f - 0.15f * static_cast<float>(slowness + 1));
    const float target = std::clamp((movementModifier + 1.0f) * 0.5f, 0.1f, 1.5f);
    previousFovMultiplier_ = fovMultiplier_;
    fovMultiplier_ += (target - fovMultiplier_) * 0.5f;
    fovMultiplier_ = std::clamp(fovMultiplier_, 0.1f, 1.5f);
}

float PlayerEntity::getFovMultiplier(float partialTick) const noexcept
{
    partialTick = std::clamp(partialTick, 0.0f, 1.0f);
    return previousFovMultiplier_ +
        (fovMultiplier_ - previousFovMultiplier_) * partialTick;
}
}
