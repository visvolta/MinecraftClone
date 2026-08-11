#pragma once

#include "entity/MobEntity.h"
#include "gameplay/GameplayRegistries.h"

#include <cstdint>
#include <random>
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

class MobEntityManager
{
public:
    explicit MobEntityManager(const gameplay::GameplayRegistries& registries);

    void tick(World& world, Player& player, std::uint64_t worldTime);
    bool attackNearest(
        const glm::vec3& origin,
        const glm::vec3& direction,
        float maximumDistance,
        float damage
    );
    void clear() noexcept;

    [[nodiscard]] const std::vector<MobEntity>& entities() const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] std::vector<MobDeath> takeDeaths();

private:
    const gameplay::GameplayRegistries* registries_ = nullptr;
    std::vector<MobEntity> entities_;
    std::vector<MobDeath> deathEvents_;
    std::mt19937 random_{1122U};
    std::uint64_t ticks_ = 0;

    void tryNaturalSpawn(
        World& world,
        const Player& player,
        gameplay::MobCategory category,
        int skylightSubtracted
    );
};
}
