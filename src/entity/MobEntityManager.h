#pragma once

#include "entity/MobEntity.h"
#include "gameplay/GameplayRegistries.h"
#include "worldgen/JavaRandom.h"

#include <cstdint>
#include <vector>

class Player;
class World;

namespace mc::entity
{
struct MobDeath
{
    core::ResourceLocation lootTable;
    glm::vec3 position{};
    bool killedByPlayer = false;
};

struct MobInteractionDrop
{
    ItemStack stack{};
    glm::vec3 position{};
};

class MobEntityManager
{
public:
    explicit MobEntityManager(const gameplay::GameplayRegistries& registries);

    void tick(
        World& world,
        Player& player,
        std::uint64_t worldTime,
        ItemType playerMainHand
    );
    bool attackNearest(
        const glm::vec3& origin,
        const glm::vec3& direction,
        float maximumDistance,
        float damage
    );
    bool interactNearest(
        const glm::vec3& origin,
        const glm::vec3& direction,
        float maximumDistance,
        Player& player,
        ItemStack& heldStack
    );
    [[nodiscard]] std::vector<MobPersistentState> persistentStates() const;
    void restorePersistentStates(
        const std::vector<MobPersistentState>& states
    );
    void clear() noexcept;

    [[nodiscard]] const std::vector<MobEntity>& entities() const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] std::vector<MobDeath> takeDeaths();
    [[nodiscard]] std::vector<MobInteractionDrop> takeInteractionDrops();

private:
    const gameplay::GameplayRegistries* registries_ = nullptr;
    std::vector<MobEntity> entities_;
    std::vector<MobDeath> deathEvents_;
    std::vector<MobInteractionDrop> interactionDrops_;
    JavaRandom random_{1122};
    std::uint64_t ticks_ = 0;

    void tryNaturalSpawn(
        World& world,
        const Player& player,
        gameplay::MobCategory category,
        int skylightSubtracted
    );
    [[nodiscard]] MobEntity* nearestAlongRay(
        const glm::vec3& origin,
        const glm::vec3& direction,
        float maximumDistance
    );
    void rebindEntities() noexcept;
};
}
