#pragma once
#include "Item.h"
#include "Raycast.h"

class ItemEntityManager;
class World;

class BlockBreakingController
{
public:
    void update(
        bool attackHeld,
        const RaycastHit& hit,
        float deltaTime,
        ItemStack& heldStack,
        bool playerOnGround,
        bool playerUnderwater,
        int hasteAmplifier,
        int fatigueAmplifier,
        bool aquaAffinity,
        World& world,
        ItemEntityManager& itemEntities
    );

    void reset() noexcept;

    [[nodiscard]] bool isBreaking() const noexcept;
    [[nodiscard]] float getProgress() const noexcept;
    [[nodiscard]] int getDestroyStage() const noexcept;
    [[nodiscard]] const glm::ivec3& getBlockPosition() const noexcept;

private:
    glm::ivec3 blockPosition_{0};
    bool hasTarget_ = false;
    float progress_ = 0.0f;
    double tickAccumulator_ = 0.0;
    int hitWaitTicks_ = 0;
};
