#pragma once

#include "ItemEntity.h"

#include <glm/glm.hpp>

#include <cstddef>
#include <memory>
#include <random>
#include <span>
#include <vector>

class Inventory;
class ItemAtlas;
struct AtmosphereState;
class ItemEntityRenderer;
class Player;
class Texture2D;
class World;

class ItemEntityManager
{
public:
    ItemEntityManager();
    ~ItemEntityManager();

    void spawnBlockDrops(
        BlockType block,
        std::uint8_t metadata,
        const ToolProperties& tool,
        const glm::ivec3& blockPosition
    );

    void spawnPlayerDrop(
        ItemStack stack,
        const glm::vec3& eyePosition,
        const glm::vec3& lookDirection
    );

    void spawnMobDrop(ItemStack stack, const glm::vec3& position);

    void spawnContainerDrops(
        std::span<const ItemStack> contents,
        const glm::ivec3& blockPosition
    );

    void tick(
        const World& world,
        const Player& player,
        Inventory& inventory
    );

    void draw(
        float partialTick,
        const glm::mat4& view,
        const glm::mat4& projection,
        const Texture2D& blockAtlas,
        const ItemAtlas& itemAtlas,
        const AtmosphereState& atmosphere
    );

    [[nodiscard]] std::size_t size() const noexcept;
    void clear() noexcept;

private:
    std::vector<ItemEntity> entities_;
    std::unique_ptr<ItemEntityRenderer> renderer_;
    std::mt19937 random_;

    float randomFloat(float minimum, float maximum);
    void spawnDropStack(
        ItemStack stack,
        const glm::ivec3& blockPosition
    );
};
