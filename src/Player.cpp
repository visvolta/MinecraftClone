#include "Player.h"

#include "BlockShape.h"
#include "FluidState.h"
#include <algorithm>
#include <cmath>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glm/geometric.hpp>

#include "Camera.h"
#include "World.h"

Player::Player(glm::vec3 feetPosition)
    : position_(feetPosition),
      previousPosition_(feetPosition),
      renderPosition_(feetPosition)
{
}

void Player::update(GLFWwindow* window, float deltaTime, const World& world, Camera& camera)
{
    if (noClip_)
    {
        updateNoClip(window, deltaTime, camera);
        syncCamera(camera);
        return;
    }

    // Avoid a huge physics catch-up after pausing, resizing, or debugging.
    deltaTime = std::clamp(deltaTime, 0.0f, 0.25f);

    const bool jumpPressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
    if (jumpPressed && !jumpWasPressed_)
        jumpQueued_ = true;
    jumpWasPressed_ = jumpPressed;

    physicsAccumulator_ += deltaTime;

    int steps = 0;
    while (physicsAccumulator_ >= PHYSICS_STEP && steps < MAX_PHYSICS_STEPS_PER_FRAME)
    {
        simulateTick(window, world, camera);
        physicsAccumulator_ -= PHYSICS_STEP;
        ++steps;
    }

    // Discard excessive backlog rather than simulating seconds of stale input.
    if (steps == MAX_PHYSICS_STEPS_PER_FRAME)
    {
        physicsAccumulator_ = 0.0f;
        previousPosition_ = position_;
    }

    updateRenderPosition();
    syncCamera(camera);
}

const glm::vec3& Player::getPosition() const
{
    return position_;
}

glm::vec3 Player::getEyePosition() const
{
    return position_ + glm::vec3(0.0f, EYE_HEIGHT, 0.0f);
}

glm::vec3 Player::getRenderPosition() const
{
    return renderPosition_;
}

glm::vec3 Player::getRenderEyePosition() const
{
    return renderPosition_ + glm::vec3(0.0f, EYE_HEIGHT, 0.0f);
}

bool Player::isGrounded() const
{
    return grounded_;
}

bool Player::overlapsBlock(
    const glm::ivec3& blockPosition,
    BlockType block) const
{
    const BlockBox playerBox{
        {position_.x - HALF_WIDTH, position_.y,
         position_.z - HALF_WIDTH},
        {position_.x + HALF_WIDTH, position_.y + HEIGHT,
         position_.z + HALF_WIDTH}
    };
    for (const BlockBox& localBox : getBlockShape(block).collisionBoxes)
    {
        if (boxesIntersect(
                playerBox,
                translateBlockBox(localBox, blockPosition)))
        {
            return true;
        }
    }
    return false;
}

int Player::getHealth() const noexcept
{
    return health_;
}

int Player::getPreviousHealth() const noexcept
{
    return previousHealth_;
}

int Player::getMaximumHealth() const noexcept
{
    return MAXIMUM_HEALTH;
}

int Player::getAir() const noexcept
{
    return air_;
}

int Player::getMaximumAir() const noexcept
{
    return MAXIMUM_AIR;
}

int Player::getHeartFlashTicks() const noexcept
{
    return heartFlashTicks_;
}

int Player::getHurtCameraTicks() const noexcept
{
    return hurtCameraTicks_;
}

int Player::getMaximumHurtCameraTicks() const noexcept
{
    return HURT_RESISTANCE_TICKS / 2;
}

int Player::getDeathTicks() const noexcept
{
    return deathTicks_;
}

float Player::getAttackedAtYaw() const noexcept
{
    return attackedAtYaw_;
}

int Player::getTicksExisted() const noexcept
{
    return ticksExisted_;
}

bool Player::isAlive() const noexcept
{
    return health_ > 0;
}

bool Player::isInWater() const noexcept
{
    return inWater_;
}

bool Player::isInLava() const noexcept
{
    return inLava_;
}

bool Player::isHeadUnderwater() const noexcept
{
    return headUnderwater_;
}

bool Player::isBurning() const noexcept
{
    return fireTicks_ > 0;
}

const mc::gameplay::SurvivalStats& Player::survival() const noexcept
{
    return survival_;
}

mc::gameplay::SurvivalStats& Player::survival() noexcept
{
    return survival_;
}

void Player::eat(int food, float saturationModifier) noexcept
{
    survival_.eat(food, saturationModifier);
}

void Player::addExperience(int amount) noexcept
{
    survival_.addExperience(amount);
}

void Player::resetAttackCooldown() noexcept
{
    survival_.resetAttackCooldown();
}

void Player::setBlocking(bool blocking) noexcept
{
    blocking_ = blocking && isAlive();
}

bool Player::isBlocking() const noexcept
{
    return blocking_;
}

void Player::damage(int amount, const glm::vec3& sourcePosition) noexcept
{
    glm::vec3 towardSource = sourcePosition - position_;
    towardSource.y = 0.0f;
    if (blocking_ && glm::dot(towardSource, towardSource) > 0.0001f)
    {
        towardSource = glm::normalize(towardSource);
        glm::vec3 facing = lookDirection_;
        facing.y = 0.0f;
        if (glm::dot(facing, facing) > 0.0001f &&
            glm::dot(glm::normalize(facing), towardSource) > 0.0f)
        {
            return;
        }
    }
    takeDamage(amount);
}

void Player::setNoClip(bool enabled) noexcept
{
    if (noClip_ == enabled)
        return;

    noClip_ = enabled;
    verticalVelocity_ = 0.0f;
    liquidHorizontalVelocity_ = {0.0f, 0.0f};
    physicsAccumulator_ = 0.0f;
    fallDistance_ = 0.0f;
    grounded_ = false;
    inWater_ = false;
    inLava_ = false;
    headUnderwater_ = false;
    blocking_ = false;
    previousPosition_ = position_;
    renderPosition_ = position_;
}

bool Player::isNoClip() const noexcept
{
    return noClip_;
}

void Player::respawn(const glm::vec3& feetPosition) noexcept
{
    position_ = feetPosition;
    previousPosition_ = feetPosition;
    renderPosition_ = feetPosition;
    verticalVelocity_ = 0.0f;
    liquidHorizontalVelocity_ = {0.0f, 0.0f};
    physicsAccumulator_ = 0.0f;
    fallDistance_ = 0.0f;

    health_ = MAXIMUM_HEALTH;
    previousHealth_ = MAXIMUM_HEALTH;
    air_ = MAXIMUM_AIR;
    heartFlashTicks_ = 0;
    hurtCameraTicks_ = 0;
    deathTicks_ = 0;
    lastDamage_ = 0;
    fireTicks_ = 0;
    attackedAtYaw_ = 0.0f;
    survival_.respawn();

    grounded_ = false;
    jumpWasPressed_ = false;
    jumpQueued_ = false;
    noClip_ = false;
    inWater_ = false;
    inLava_ = false;
    headUnderwater_ = false;
    sprinting_ = false;
    blocking_ = false;
}

PlayerPersistentState Player::persistentState() const noexcept
{
    return {
        position_, health_, previousHealth_, air_, fireTicks_, ticksExisted_,
        survival_.persistentState()
    };
}

void Player::restorePersistentState(
    const PlayerPersistentState& state) noexcept
{
    position_ = state.position;
    previousPosition_ = position_;
    renderPosition_ = position_;
    health_ = std::clamp(state.health, 0, MAXIMUM_HEALTH);
    previousHealth_ = std::clamp(state.previousHealth, 0, MAXIMUM_HEALTH);
    air_ = std::clamp(state.air, 0, MAXIMUM_AIR);
    fireTicks_ = std::max(0, state.fireTicks);
    ticksExisted_ = std::max(0, state.ticksExisted);
    survival_.restorePersistentState(state.survival);
    verticalVelocity_ = 0.0f;
    liquidHorizontalVelocity_ = {0.0f, 0.0f};
    physicsAccumulator_ = 0.0f;
    fallDistance_ = 0.0f;
    heartFlashTicks_ = 0;
    hurtCameraTicks_ = 0;
    deathTicks_ = health_ == 0 ? 1 : 0;
    lastDamage_ = 0;
    attackedAtYaw_ = 0.0f;
    grounded_ = false;
    jumpWasPressed_ = false;
    jumpQueued_ = false;
    noClip_ = false;
    inWater_ = false;
    inLava_ = false;
    headUnderwater_ = false;
    sprinting_ = false;
    blocking_ = false;
}

void Player::updateNoClip(
    GLFWwindow* window,
    float deltaTime,
    const Camera& camera)
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
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS)
    {
        direction.y -= 1.0f;
    }

    if (glm::dot(direction, direction) > 0.000001f)
        direction = glm::normalize(direction);

    constexpr float flySpeed = 12.0f;
    position_ += direction * flySpeed * deltaTime;
    previousPosition_ = position_;
    renderPosition_ = position_;
}

void Player::simulateTick(GLFWwindow* window, const World& world, const Camera& camera)
{
    previousPosition_ = position_;
    ++ticksExisted_;

    updateEnvironment(world);
    updateDamageAndAir();

    glm::vec3 forward = camera.getForward();
    lookDirection_ = forward;
    forward.y = 0.0f;
    if (glm::dot(forward, forward) > 0.000001f)
        forward = glm::normalize(forward);

    glm::vec3 right = camera.getRight();
    right.y = 0.0f;
    if (glm::dot(right, right) > 0.000001f)
        right = glm::normalize(right);

    glm::vec3 wishDirection(0.0f);

    if (isAlive())
    {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            wishDirection += forward;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            wishDirection -= forward;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            wishDirection += right;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            wishDirection -= right;
    }

    if (glm::dot(wishDirection, wishDirection) > 1.0f)
        wishDirection = glm::normalize(wishDirection);

    sprinting_ = isAlive() && !inWater_ && !inLava_ &&
        survival_.foodLevel() > 6 &&
        (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
         glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS) &&
        glm::dot(wishDirection, wishDirection) > 0.01f;

    const glm::vec3 movementStart = position_;
    if (inWater_)
    {
        simulateLiquidMovement(
            window,
            wishDirection,
            world,
            WATER_DRAG
        );
    }
    else if (inLava_)
    {
        simulateLiquidMovement(
            window,
            wishDirection,
            world,
            LAVA_DRAG
        );
    }
    else
    {
        float speedMultiplier = sprinting_ ? 1.3f : 1.0f;
        const int speed = survival_.effectAmplifier(
            mc::gameplay::StatusEffectType::Speed
        );
        const int slowness = survival_.effectAmplifier(
            mc::gameplay::StatusEffectType::Slowness
        );
        if (speed >= 0)
            speedMultiplier *= 1.0f + 0.2f * static_cast<float>(speed + 1);
        if (slowness >= 0)
            speedMultiplier *= std::max(
                0.0f, 1.0f - 0.15f * static_cast<float>(slowness + 1)
            );
        simulateNormalMovement(wishDirection, world, speedMultiplier);
    }

    const glm::vec2 horizontalMovement(
        position_.x - movementStart.x,
        position_.z - movementStart.z
    );
    const float distance = glm::length(horizontalMovement);
    if (distance > 0.0f)
    {
        if (inWater_)
            survival_.addExhaustion(distance * 0.01f);
        else if (sprinting_)
            survival_.addExhaustion(distance * 0.1f);
        else
            survival_.addExhaustion(distance * 0.01f);
    }
    survival_.tick(true, true, health_, MAXIMUM_HEALTH);

    // Refresh these after movement so rendering and mining use the player's
    // new fixed-tick position rather than the previous tick's liquid state.
    updateEnvironment(world);
}

void Player::simulateNormalMovement(
    const glm::vec3& wishDirection,
    const World& world,
    float speedMultiplier)
{
    liquidHorizontalVelocity_ = {0.0f, 0.0f};

    const glm::vec3 horizontalDisplacement =
        wishDirection * WALK_SPEED * speedMultiplier * PHYSICS_STEP;

    moveAxis(horizontalDisplacement.x, 0, world);
    moveAxis(horizontalDisplacement.z, 2, world);

    const bool jumpedThisTick = jumpQueued_ && grounded_ && isAlive();
    if (jumpedThisTick)
    {
        verticalVelocity_ = JUMP_VELOCITY_PER_TICK;
        grounded_ = false;
        fallDistance_ = 0.0f;
        survival_.addExhaustion(sprinting_ ? 0.2f : 0.05f);
    }
    else
    {
        // Java-style living-entity vertical update:
        // motionY = (motionY - 0.08) * 0.98
        verticalVelocity_ =
            (verticalVelocity_ - GRAVITY_PER_TICK) * VERTICAL_DRAG;
        verticalVelocity_ =
            std::max(verticalVelocity_, TERMINAL_VELOCITY_PER_TICK);
    }
    jumpQueued_ = false;

    grounded_ = false;
    const float movedY = moveAxis(verticalVelocity_, 1, world);

    if (grounded_)
    {
        applyFallDamage();
        fallDistance_ = 0.0f;
    }
    else if (movedY < 0.0f)
    {
        fallDistance_ -= movedY;
    }
}

void Player::simulateLiquidMovement(
    GLFWwindow* window,
    const glm::vec3& wishDirection,
    const World& world,
    float drag)
{
    const BlockType liquid = inWater_ ? BlockType::Water : BlockType::Lava;
    const glm::vec3 current = liquidFlowAt(
        position_,
        world,
        liquid,
        liquid == BlockType::Lava
    );
    constexpr float currentAcceleration = 0.014f;
    liquidHorizontalVelocity_.x += current.x * currentAcceleration;
    liquidHorizontalVelocity_.y += current.z * currentAcceleration;
    verticalVelocity_ += current.y * currentAcceleration;

    liquidHorizontalVelocity_.x +=
        wishDirection.x * LIQUID_ACCELERATION_PER_TICK;
    liquidHorizontalVelocity_.y +=
        wishDirection.z * LIQUID_ACCELERATION_PER_TICK;

    if (isAlive() &&
        glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
    {
        verticalVelocity_ += LIQUID_JUMP_PER_TICK;
    }
    jumpQueued_ = false;
    fallDistance_ = 0.0f;

    const float intendedX = liquidHorizontalVelocity_.x;
    const float intendedZ = liquidHorizontalVelocity_.y;
    const float movedX = moveAxis(
        intendedX,
        0,
        world
    );
    const float movedZ = moveAxis(
        intendedZ,
        2,
        world
    );

    const bool collidedHorizontally =
        std::abs(movedX - intendedX) > COLLISION_EPSILON ||
        std::abs(movedZ - intendedZ) > COLLISION_EPSILON;

    grounded_ = false;
    moveAxis(verticalVelocity_, 1, world);

    if (collidedHorizontally && canSwimStep(world, liquid))
        verticalVelocity_ = SWIM_STEP_VELOCITY_PER_TICK;

    liquidHorizontalVelocity_ *= drag;
    verticalVelocity_ *= drag;
    verticalVelocity_ -= LIQUID_GRAVITY_PER_TICK;
}

void Player::updateEnvironment(const World& world)
{
    inWater_ = intersectsLiquidAt(
        position_,
        world,
        BlockType::Water,
        false
    );
    inLava_ = intersectsLiquidAt(
        position_,
        world,
        BlockType::Lava,
        true
    );
    headUnderwater_ = eyeIsSubmerged(world, BlockType::Water);
}

void Player::updateDamageAndAir()
{
    // Water immediately extinguishes the entity and cancels fall distance.
    if (inWater_)
    {
        fireTicks_ = 0;
        fallDistance_ = 0.0f;
    }

    // Beta applies one point of fire damage every second while burning.
    if (fireTicks_ > 0)
    {
        if (fireTicks_ % 20 == 0)
            takeDamage(1);
        --fireTicks_;
    }

    // Entity.setOnFireFromLava deals four points and refreshes 30 seconds of
    // burning. Hurt resistance naturally limits repeated contact damage.
    if (inLava_)
    {
        takeDamage(4);
        fireTicks_ = 600;
    }

    if (headUnderwater_ && isAlive())
    {
        --air_;
        if (air_ == -20)
        {
            air_ = 0;
            takeDamage(2);
        }
        fireTicks_ = 0;
    }
    else
    {
        air_ = MAXIMUM_AIR;
    }

    if (heartFlashTicks_ > 0)
        --heartFlashTicks_;
    if (hurtCameraTicks_ > 0)
        --hurtCameraTicks_;

    if (!isAlive())
        ++deathTicks_;
}

void Player::takeDamage(int amount) noexcept
{
    if (amount <= 0 || !isAlive())
        return;

    const float armor = static_cast<float>(survival_.armorPoints());
    const float toughness = survival_.armorToughness();
    const float divisor = 2.0f + toughness / 4.0f;
    const float effectiveArmor = std::clamp(
        armor - static_cast<float>(amount) / divisor,
        armor * 0.2f,
        20.0f
    );
    amount = std::max(1, static_cast<int>(std::ceil(
        static_cast<float>(amount) * (1.0f - effectiveArmor / 25.0f)
    )));
    int appliedDamage = amount;

    // Beta's heartsLife rule ignores equal/weaker hits during the first half
    // of the 20-tick hurt window, but still applies the extra part of a
    // stronger hit.
    if (heartFlashTicks_ > HURT_RESISTANCE_TICKS / 2)
    {
        if (amount <= lastDamage_)
            return;

        appliedDamage = amount - lastDamage_;
        lastDamage_ = amount;
    }
    else
    {
        lastDamage_ = amount;
        previousHealth_ = health_;
        heartFlashTicks_ = HURT_RESISTANCE_TICKS;
        hurtCameraTicks_ = HURT_RESISTANCE_TICKS / 2;

        // Environmental damage has no attacker. Beta chooses one of the two
        // opposite directions so the view still receives a visible roll.
        const std::uint32_t directionSeed =
            static_cast<std::uint32_t>(ticksExisted_) * 1103515245U + 12345U;
        attackedAtYaw_ = (directionSeed & 0x10000U) != 0U
            ? 180.0f
            : 0.0f;
    }

    health_ = std::max(0, health_ - appliedDamage);
}

void Player::applyFallDamage() noexcept
{
    const int damage = static_cast<int>(
        std::ceil(fallDistance_ - 3.0f)
    );

    if (damage > 0)
        takeDamage(damage);
}

void Player::moveAndCollide(const glm::vec3& displacement, const World& world)
{
    moveAxis(displacement.x, 0, world);
    moveAxis(displacement.y, 1, world);
    moveAxis(displacement.z, 2, world);
}

float Player::moveAxis(float amount, int axis, const World& world)
{
    if (amount == 0.0f)
        return 0.0f;

    const float startingCoordinate = position_[axis];
    glm::vec3 candidate = position_;
    candidate[axis] += amount;

    if (!collidesAt(candidate, world))
    {
        position_ = candidate;
        return amount;
    }

    // Find the furthest safe point along this axis. Axis-separated movement
    // allows the player to slide naturally along walls.
    float safe = 0.0f;
    float blocked = amount;

    for (int iteration = 0; iteration < 18; ++iteration)
    {
        const float midpoint = (safe + blocked) * 0.5f;
        candidate = position_;
        candidate[axis] += midpoint;

        if (collidesAt(candidate, world))
            blocked = midpoint;
        else
            safe = midpoint;
    }

    position_[axis] += safe;

    if (axis == 1)
    {
        if (amount < 0.0f)
            grounded_ = true;

        verticalVelocity_ = 0.0f;
    }
    else if (axis == 0)
    {
        liquidHorizontalVelocity_.x = 0.0f;
    }
    else if (axis == 2)
    {
        liquidHorizontalVelocity_.y = 0.0f;
    }

    return position_[axis] - startingCoordinate;
}

bool Player::collidesAt(const glm::vec3& position, const World& world) const
{
    const float minX = position.x - HALF_WIDTH + COLLISION_EPSILON;
    const float maxX = position.x + HALF_WIDTH - COLLISION_EPSILON;
    const float minY = position.y + COLLISION_EPSILON;
    const float maxY = position.y + HEIGHT - COLLISION_EPSILON;
    const float minZ = position.z - HALF_WIDTH + COLLISION_EPSILON;
    const float maxZ = position.z + HALF_WIDTH - COLLISION_EPSILON;

    const int blockMinX = static_cast<int>(std::floor(minX));
    const int blockMaxX = static_cast<int>(std::floor(maxX));
    const int blockMinY = static_cast<int>(std::floor(minY));
    const int blockMaxY = static_cast<int>(std::floor(maxY));
    const int blockMinZ = static_cast<int>(std::floor(minZ));
    const int blockMaxZ = static_cast<int>(std::floor(maxZ));
    const BlockBox playerBox{
        {minX, minY, minZ},
        {maxX, maxY, maxZ}
    };

    for (int y = blockMinY; y <= blockMaxY; ++y)
    {
        for (int z = blockMinZ; z <= blockMaxZ; ++z)
        {
            for (int x = blockMinX; x <= blockMaxX; ++x)
            {
                const BlockType block = world.getBlock(x, y, z);
                for (const BlockBox& localBox :
                     getBlockShape(block).collisionBoxes)
                {
                    if (boxesIntersect(
                            playerBox,
                            translateBlockBox(localBox, {x, y, z})))
                    {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

bool Player::intersectsLiquidAt(
    const glm::vec3& position,
    const World& world,
    BlockType liquid,
    bool contractedHorizontally) const
{
    const float horizontalInset = contractedHorizontally ? 0.1f : 0.0f;
    const float liquidHalfWidth = HALF_WIDTH - horizontalInset;

    const float minX = position.x - liquidHalfWidth + COLLISION_EPSILON;
    const float maxX = position.x + liquidHalfWidth - COLLISION_EPSILON;
    const float minY = position.y - 0.4f + COLLISION_EPSILON;
    const float maxY = position.y + HEIGHT - COLLISION_EPSILON;
    const float minZ = position.z - liquidHalfWidth + COLLISION_EPSILON;
    const float maxZ = position.z + liquidHalfWidth - COLLISION_EPSILON;

    const int blockMinX = static_cast<int>(std::floor(minX));
    const int blockMaxX = static_cast<int>(std::floor(maxX));
    const int blockMinY = static_cast<int>(std::floor(minY));
    const int blockMaxY = static_cast<int>(std::floor(maxY));
    const int blockMinZ = static_cast<int>(std::floor(minZ));
    const int blockMaxZ = static_cast<int>(std::floor(maxZ));

    for (int y = blockMinY; y <= blockMaxY; ++y)
    {
        for (int z = blockMinZ; z <= blockMaxZ; ++z)
        {
            for (int x = blockMinX; x <= blockMaxX; ++x)
            {
                if (world.getBlock(x, y, z) != liquid)
                    continue;

                const float surface = static_cast<float>(y) +
                    1.0f - FluidState::airFraction(
                        world.getBlockMetadata(x, y, z)
                    );
                if (minY < surface && maxY > static_cast<float>(y))
                    return true;
            }
        }
    }

    return false;
}

bool Player::eyeIsSubmerged(
    const World& world,
    BlockType liquid) const
{
    const glm::vec3 eye = getEyePosition();
    const int blockX = static_cast<int>(std::floor(eye.x));
    const int blockY = static_cast<int>(std::floor(eye.y));
    const int blockZ = static_cast<int>(std::floor(eye.z));

    if (world.getBlock(blockX, blockY, blockZ) != liquid)
        return false;

    const float height = 1.0f - FluidState::airFraction(
        world.getBlockMetadata(blockX, blockY, blockZ)
    );
    return eye.y < static_cast<float>(blockY) + height;
}

glm::vec3 Player::liquidFlowAt(
    const glm::vec3& position,
    const World& world,
    BlockType liquid,
    bool contractedHorizontally) const
{
    const float inset = contractedHorizontally ? 0.1f : 0.0f;
    const float halfWidth = HALF_WIDTH - inset;
    const float minX = position.x - halfWidth + COLLISION_EPSILON;
    const float maxX = position.x + halfWidth - COLLISION_EPSILON;
    const float minY = position.y - 0.4f + COLLISION_EPSILON;
    const float maxY = position.y + HEIGHT - COLLISION_EPSILON;
    const float minZ = position.z - halfWidth + COLLISION_EPSILON;
    const float maxZ = position.z + halfWidth - COLLISION_EPSILON;

    glm::vec3 flow(0.0f);
    for (int y = static_cast<int>(std::floor(minY));
         y <= static_cast<int>(std::floor(maxY));
         ++y)
    {
        for (int z = static_cast<int>(std::floor(minZ));
             z <= static_cast<int>(std::floor(maxZ));
             ++z)
        {
            for (int x = static_cast<int>(std::floor(minX));
                 x <= static_cast<int>(std::floor(maxX));
                 ++x)
            {
                if (world.getBlock(x, y, z) != liquid)
                    continue;

                const float surface = static_cast<float>(y) +
                    1.0f - FluidState::airFraction(
                        world.getBlockMetadata(x, y, z)
                    );
                if (minY < surface && maxY > static_cast<float>(y))
                    flow += world.getFluidFlowVector(x, y, z, liquid);
            }
        }
    }

    const float length = glm::length(flow);
    return length > 0.00001f ? flow / length : glm::vec3(0.0f);
}

bool Player::canSwimStep(
    const World& world,
    BlockType liquid) const
{
    glm::vec3 raisedPosition = position_;
    raisedPosition.y += 0.6f;

    return !collidesAt(raisedPosition, world) &&
           intersectsLiquidAt(
               raisedPosition,
               world,
               liquid,
               liquid == BlockType::Lava
           );
}

void Player::updateRenderPosition()
{
    const float alpha = std::clamp(physicsAccumulator_ / PHYSICS_STEP, 0.0f, 1.0f);
    renderPosition_ = previousPosition_ + (position_ - previousPosition_) * alpha;
}

void Player::syncCamera(Camera& camera) const
{
    camera.setPosition(getRenderEyePosition());
}
