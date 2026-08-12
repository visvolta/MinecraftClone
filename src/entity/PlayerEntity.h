#pragma once

#include "entity/LivingEntity.h"
#include "gameplay/SurvivalStats.h"

#include <glm/glm.hpp>

struct GLFWwindow;
class Camera;
class World;

namespace mc::entity
{
class PlayerEntity : public LivingEntity
{
public:
    explicit PlayerEntity(World& world, glm::vec3 feetPosition = {0.5f, 14.0f, 0.5f});

    void update(GLFWwindow* window, float deltaTime, Camera& camera);
    [[nodiscard]] bool isPlayer() const noexcept override { return true; }
    [[nodiscard]] EntityKind entityKind() const noexcept override
    {
        return EntityKind::Player;
    }
    [[nodiscard]] core::ResourceLocation getType() const override;
    [[nodiscard]] float getEyeHeight() const override;

    [[nodiscard]] const glm::vec3& getPosition() const;
    [[nodiscard]] glm::vec3 getEyePosition() const;
    [[nodiscard]] glm::vec3 getRenderPosition() const;
    [[nodiscard]] glm::vec3 getRenderEyePosition() const;
    [[nodiscard]] const glm::vec3& getLookDirection() const noexcept;
    [[nodiscard]] bool isGrounded() const noexcept { return onGround; }
    [[nodiscard]] bool overlapsBlock(const glm::ivec3& blockPosition, BlockType block) const;

    [[nodiscard]] int getHealth() const noexcept;
    [[nodiscard]] int getPreviousHealth() const noexcept { return previousHealth_; }
    [[nodiscard]] int getMaximumHealth() const noexcept;
    [[nodiscard]] int getAir() const noexcept { return air_; }
    [[nodiscard]] int getMaximumAir() const noexcept { return 300; }
    [[nodiscard]] int getHeartFlashTicks() const noexcept { return hurtResistantTime; }
    [[nodiscard]] int getHurtCameraTicks() const noexcept { return hurtTime; }
    [[nodiscard]] int getMaximumHurtCameraTicks() const noexcept { return 10; }
    [[nodiscard]] int getDeathTicks() const noexcept { return deathTime; }
    [[nodiscard]] float getAttackedAtYaw() const noexcept { return attackedAtYaw; }
    [[nodiscard]] int getTicksExisted() const noexcept { return ticksExisted_; }
    [[nodiscard]] bool isHeadUnderwater() const noexcept { return headUnderwater_; }
    [[nodiscard]] bool isCrouching() const noexcept { return isSneaking(); }
    [[nodiscard]] float getFovMultiplier(float partialTick) const noexcept;
    [[nodiscard]] const gameplay::SurvivalStats& survival() const noexcept { return survival_; }
    [[nodiscard]] gameplay::SurvivalStats& survival() noexcept { return survival_; }
    void eat(int food, float saturationModifier) noexcept;
    void addExperience(int amount) noexcept;
    void resetAttackCooldown() noexcept;
    void setBlocking(bool blocking) noexcept;
    [[nodiscard]] bool isBlocking() const noexcept { return blocking_; }
    [[nodiscard]] ItemType getHeldItemType() const noexcept { return heldItemType_; }
    void setHeldItemType(ItemType item) noexcept { heldItemType_ = item; }

    void setNoClip(bool enabled) noexcept;
    [[nodiscard]] bool isNoClip() const noexcept { return noClip; }
    void respawn(const glm::vec3& feetPosition) noexcept;
    void setRidingPosition(const glm::vec3& feetPosition) noexcept;

    struct PersistentState
    {
        glm::vec3 position{0.5f, 14.0f, 0.5f};
        int health = 20;
        int previousHealth = 20;
        int air = 300;
        int fireTicks = 0;
        int ticksExisted = 0;
        EntityUuid uuid{};
        gameplay::SurvivalPersistentState survival;
    };
    [[nodiscard]] PersistentState persistentState() const noexcept;
    void restorePersistentState(const PersistentState& state) noexcept;

    bool attackEntityFrom(const DamageSource& source, float amount) override;
    void damage(int amount, const glm::vec3& sourcePosition) noexcept;

protected:
    bool canBlockDamageSource(const DamageSource& source) const override;
    void updateEntityActionState() override;

private:
    glm::vec3 lookDirection_{0.0f, 0.0f, 1.0f};
    glm::vec3 cachedPosition_{};
    glm::vec3 renderPosition_{};
    int previousHealth_ = 20;
    bool headUnderwater_ = false;
    bool blocking_ = false;
    bool jumpQueued_ = false;
    bool jumpWasPressed_ = false;
    bool forwardKeyWasDown_ = false;
    int sprintToggleTicks_ = 0;
    float previousFovMultiplier_ = 1.0f;
    float fovMultiplier_ = 1.0f;
    float physicsAccumulator_ = 0.0f;
    ItemType heldItemType_ = ItemType::Empty;
    gameplay::SurvivalStats survival_;

    void simulateInput(GLFWwindow* window, Camera& camera);
    void updateEnvironment();
    void updateFovMultiplier() noexcept;
};
}
