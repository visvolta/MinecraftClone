#pragma once

#include "Block.h"
#include "gameplay/SurvivalStats.h"

#include <glm/glm.hpp>

struct GLFWwindow;
class Camera;
class World;

struct PlayerPersistentState
{
    glm::vec3 position{0.5f, 14.0f, 0.5f};
    int health = 20;
    int previousHealth = 20;
    int air = 300;
    int fireTicks = 0;
    int ticksExisted = 0;
    mc::gameplay::SurvivalPersistentState survival;
};

class Player
{
public:
    // Position is the centre of the player's feet, matching Minecraft entity coordinates.
    explicit Player(glm::vec3 feetPosition = {0.5f, 14.0f, 0.5f});

    void update(GLFWwindow* window, float deltaTime, const World& world, Camera& camera);

    [[nodiscard]] const glm::vec3& getPosition() const;
    [[nodiscard]] glm::vec3 getEyePosition() const;
    [[nodiscard]] glm::vec3 getRenderPosition() const;
    [[nodiscard]] glm::vec3 getRenderEyePosition() const;
    [[nodiscard]] bool isGrounded() const;
    [[nodiscard]] bool overlapsBlock(
        const glm::ivec3& blockPosition,
        BlockType block
    ) const;

    [[nodiscard]] int getHealth() const noexcept;
    [[nodiscard]] int getPreviousHealth() const noexcept;
    [[nodiscard]] int getMaximumHealth() const noexcept;
    [[nodiscard]] int getAir() const noexcept;
    [[nodiscard]] int getMaximumAir() const noexcept;
    [[nodiscard]] int getHeartFlashTicks() const noexcept;
    [[nodiscard]] int getHurtCameraTicks() const noexcept;
    [[nodiscard]] int getMaximumHurtCameraTicks() const noexcept;
    [[nodiscard]] int getDeathTicks() const noexcept;
    [[nodiscard]] float getAttackedAtYaw() const noexcept;
    [[nodiscard]] int getTicksExisted() const noexcept;
    [[nodiscard]] bool isAlive() const noexcept;
    [[nodiscard]] bool isInWater() const noexcept;
    [[nodiscard]] bool isInLava() const noexcept;
    [[nodiscard]] bool isHeadUnderwater() const noexcept;
    [[nodiscard]] bool isBurning() const noexcept;
    [[nodiscard]] bool isSprinting() const noexcept;
    [[nodiscard]] bool isCrouching() const noexcept;
    [[nodiscard]] float getFovMultiplier(float partialTick) const noexcept;
    [[nodiscard]] const mc::gameplay::SurvivalStats& survival() const noexcept;
    [[nodiscard]] mc::gameplay::SurvivalStats& survival() noexcept;
    void eat(int food, float saturationModifier) noexcept;
    void addExperience(int amount) noexcept;
    void resetAttackCooldown() noexcept;
    void setBlocking(bool blocking) noexcept;
    [[nodiscard]] bool isBlocking() const noexcept;
    void damage(int amount, const glm::vec3& sourcePosition) noexcept;

    void setNoClip(bool enabled) noexcept;
    [[nodiscard]] bool isNoClip() const noexcept;
    void respawn(const glm::vec3& feetPosition) noexcept;
    [[nodiscard]] PlayerPersistentState persistentState() const noexcept;
    void restorePersistentState(const PlayerPersistentState& state) noexcept;

private:
    // Standing Java player dimensions: 0.6 blocks wide, 1.8 blocks tall,
    // with the camera/eyes 1.62 blocks above the feet.
    static constexpr float WIDTH = 0.6f;
    static constexpr float HEIGHT = 1.8f;
    static constexpr float CROUCH_HEIGHT = 1.65f;
    static constexpr float EYE_HEIGHT = 1.62f;
    static constexpr float CROUCH_EYE_HEIGHT = 1.54f;
    static constexpr float HALF_WIDTH = WIDTH * 0.5f;

    static constexpr float WALK_SPEED = 4.317f; // blocks per second

    // Minecraft updates entity physics at 20 ticks per second. Vertical motion
    // is stored in blocks per tick so the jump and falling curve match Java's
    // tick formula instead of approximating it with continuous gravity.
    static constexpr float PHYSICS_STEP = 1.0f / 20.0f;
    static constexpr float GRAVITY_PER_TICK = 0.08f;
    static constexpr float VERTICAL_DRAG = 0.98f;
    static constexpr float JUMP_VELOCITY_PER_TICK = 0.42f;
    static constexpr float TERMINAL_VELOCITY_PER_TICK = -3.92f;

    // EntityLiving.moveEntityWithHeading liquid constants from Beta 1.7.3.
    static constexpr float LIQUID_ACCELERATION_PER_TICK = 0.02f;
    static constexpr float LIQUID_JUMP_PER_TICK = 0.04f;
    static constexpr float LIQUID_GRAVITY_PER_TICK = 0.02f;
    static constexpr float WATER_DRAG = 0.8f;
    static constexpr float LAVA_DRAG = 0.5f;
    static constexpr float SWIM_STEP_VELOCITY_PER_TICK = 0.3f;

    static constexpr int MAXIMUM_HEALTH = 20;
    static constexpr int MAXIMUM_AIR = 300;
    static constexpr int HURT_RESISTANCE_TICKS = 20;

    static constexpr float COLLISION_EPSILON = 0.0001f;
    static constexpr int MAX_PHYSICS_STEPS_PER_FRAME = 5;

    glm::vec3 position_;
    glm::vec3 previousPosition_;
    glm::vec3 renderPosition_;
    float verticalVelocity_ = 0.0f; // blocks per tick
    glm::vec2 liquidHorizontalVelocity_{0.0f}; // blocks per tick
    float physicsAccumulator_ = 0.0f;

    float fallDistance_ = 0.0f;
    int health_ = MAXIMUM_HEALTH;
    int previousHealth_ = MAXIMUM_HEALTH;
    int air_ = MAXIMUM_AIR;
    int heartFlashTicks_ = 0;
    int hurtCameraTicks_ = 0;
    int deathTicks_ = 0;
    int lastDamage_ = 0;
    int fireTicks_ = 0;
    int ticksExisted_ = 0;
    float attackedAtYaw_ = 0.0f;

    bool grounded_ = false;
    bool jumpWasPressed_ = false;
    bool jumpQueued_ = false;
    bool noClip_ = false;
    bool inWater_ = false;
    bool inLava_ = false;
    bool headUnderwater_ = false;
    bool sprinting_ = false;
    bool crouching_ = false;
    bool forwardKeyWasDown_ = false;
    bool collidedHorizontally_ = false;
    int sprintToggleTicks_ = 0;
    bool blocking_ = false;
    float previousFovMultiplier_ = 1.0f;
    float fovMultiplier_ = 1.0f;
    glm::vec3 lookDirection_{0.0f, 0.0f, 1.0f};
    mc::gameplay::SurvivalStats survival_;

    void simulateTick(GLFWwindow* window, const World& world, const Camera& camera);
    void simulateNormalMovement(
        const glm::vec3& wishDirection,
        const World& world,
        float speedMultiplier
    );
    void simulateLiquidMovement(
        GLFWwindow* window,
        const glm::vec3& wishDirection,
        const World& world,
        float drag
    );
    void updateEnvironment(const World& world);
    void updateDamageAndAir();
    void updateFovMultiplier() noexcept;
    void takeDamage(int amount) noexcept;
    void applyFallDamage() noexcept;
    void updateNoClip(GLFWwindow* window, float deltaTime, const Camera& camera);
    void moveAndCollide(const glm::vec3& displacement, const World& world);
    float moveAxis(float amount, int axis, const World& world);
    [[nodiscard]] bool collidesAt(const glm::vec3& position, const World& world) const;
    [[nodiscard]] bool collidesAtHeight(
        const glm::vec3& position,
        float height,
        const World& world
    ) const;
    [[nodiscard]] glm::vec2 clampCrouchingMovement(
        glm::vec2 displacement,
        const World& world
    ) const;
    [[nodiscard]] float currentHeight() const noexcept;
    [[nodiscard]] float currentEyeHeight() const noexcept;
    [[nodiscard]] bool intersectsLiquidAt(
        const glm::vec3& position,
        const World& world,
        BlockType liquid,
        bool contractedHorizontally
    ) const;
    [[nodiscard]] bool eyeIsSubmerged(
        const World& world,
        BlockType liquid
    ) const;
    [[nodiscard]] glm::vec3 liquidFlowAt(
        const glm::vec3& position,
        const World& world,
        BlockType liquid,
        bool contractedHorizontally
    ) const;
    [[nodiscard]] bool canSwimStep(
        const World& world,
        BlockType liquid
    ) const;
    void updateRenderPosition();
    void syncCamera(Camera& camera) const;
};
