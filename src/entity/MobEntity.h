#pragma once

#include "core/ResourceLocation.h"

#include <glm/glm.hpp>

#include <random>

class Player;
class World;

namespace mc::gameplay { struct MobDefinition; }

namespace mc::entity
{
struct MobAnimationState
{
    float age = 0.0f;
    float limbSwing = 0.0f;
    float limbSwingAmount = 0.0f;
    float headYaw = 0.0f;
    float headPitch = 0.0f;
    float attackProgress = 0.0f;
    float jumpProgress = 0.0f;
    float hurtProgress = 0.0f;
    float deathProgress = 0.0f;
    bool onGround = true;
    bool inWater = false;
    bool aggressive = false;
};

class MobEntity
{
public:
    MobEntity(
        core::ResourceLocation type,
        const gameplay::MobDefinition& definition,
        glm::vec3 position,
        float yaw
    );

    void tick(
        World& world,
        Player& player,
        bool daytime,
        std::mt19937& random
    );
    void damage(float amount, bool causedByPlayer = false) noexcept;

    [[nodiscard]] const core::ResourceLocation& type() const noexcept;
    [[nodiscard]] const core::ResourceLocation& texture() const noexcept;
    [[nodiscard]] const core::ResourceLocation& overlayTexture() const noexcept;
    [[nodiscard]] const glm::vec3& overlayColour() const noexcept;
    [[nodiscard]] const gameplay::MobDefinition& definition() const noexcept;
    [[nodiscard]] const glm::vec3& position() const noexcept;
    [[nodiscard]] glm::vec3 interpolatedPosition(float partialTick) const noexcept;
    [[nodiscard]] float yaw() const noexcept;
    [[nodiscard]] float interpolatedYaw(float partialTick) const noexcept;
    [[nodiscard]] float health() const noexcept;
    [[nodiscard]] int age() const noexcept;
    [[nodiscard]] bool dead() const noexcept;
    [[nodiscard]] bool killedByPlayer() const noexcept;
    [[nodiscard]] int deathTicks() const noexcept;
    [[nodiscard]] MobAnimationState animationState(
        float partialTick
    ) const noexcept;

private:
    core::ResourceLocation type_;
    core::ResourceLocation texture_{"minecraft:entity/zombie/zombie"};
    core::ResourceLocation overlayTexture_{"minecraft:entity/empty"};
    glm::vec3 overlayColour_{1.0f};
    const gameplay::MobDefinition* definition_ = nullptr;
    glm::vec3 position_{};
    glm::vec3 previousPosition_{};
    glm::vec3 velocity_{};
    glm::vec2 wanderDirection_{};
    float yaw_ = 0.0f;
    float previousYaw_ = 0.0f;
    float headYaw_ = 0.0f;
    float headPitch_ = 0.0f;
    float limbSwing_ = 0.0f;
    float previousLimbSwing_ = 0.0f;
    float limbSwingAmount_ = 0.0f;
    float previousLimbSwingAmount_ = 0.0f;
    float health_ = 20.0f;
    int age_ = 0;
    int wanderTicks_ = 0;
    int attackCooldown_ = 0;
    int fireTicks_ = 0;
    int hurtTicks_ = 0;
    int panicTicks_ = 0;
    int revengeTicks_ = 0;
    int fuseTicks_ = 0;
    int deathTicks_ = 0;
    int eatTicks_ = 0;
    bool onGround_ = false;
    bool inWater_ = false;
    bool aggressive_ = false;
    bool killedByPlayer_ = false;

    [[nodiscard]] bool collides(
        const World& world,
        const glm::vec3& position
    ) const;
    void moveAxis(const World& world, float amount, int axis);
};
}
