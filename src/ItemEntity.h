#pragma once

#include "Inventory.h"
#include "core/ResourceLocation.h"

#include <glm/glm.hpp>

class World;

class ItemEntity
{
public:
    [[nodiscard]] static const mc::core::ResourceLocation& typeId() noexcept;

    ItemEntity(
        const glm::vec3& position,
        ItemStack stack,
        const glm::vec3& velocity,
        float hoverStart,
        int pickupDelay
    );

    void tick(const World& world);

    [[nodiscard]] bool isDead() const noexcept;
    void kill() noexcept;

    [[nodiscard]] bool canBePickedUp() const noexcept;
    [[nodiscard]] bool isNear(const glm::vec3& point) const noexcept;

    [[nodiscard]] const glm::vec3& getPosition() const noexcept;
    [[nodiscard]] const glm::vec3& getPreviousPosition() const noexcept;
    [[nodiscard]] glm::vec3 getInterpolatedPosition(float partialTick) const;

    [[nodiscard]] const ItemStack& getStack() const noexcept;
    [[nodiscard]] ItemStack& getStack() noexcept;

    [[nodiscard]] int getAge() const noexcept;
    [[nodiscard]] float getHoverStart() const noexcept;

private:
    static constexpr float HALF_SIZE = 0.125f;

    glm::vec3 position_{};
    glm::vec3 previousPosition_{};
    glm::vec3 velocity_{};
    ItemStack stack_{};

    int age_ = 0;
    int pickupDelay_ = 10;
    bool dead_ = false;
    bool onGround_ = false;
    float hoverStart_ = 0.0f;

    [[nodiscard]] bool collides(
        const World& world,
        const glm::vec3& position
    ) const;

    void moveAxis(
        const World& world,
        float amount,
        int axis
    );
};
