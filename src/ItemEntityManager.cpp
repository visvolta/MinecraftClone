#include "ItemEntityManager.h"

#include "Inventory.h"
#include "Atmosphere.h"
#include "BlockDrops.h"
#include "ItemEntityRenderer.h"
#include "ItemAtlas.h"
#include "Player.h"
#include "Texture2D.h"
#include "World.h"
#include "content/ContentCatalog.h"

#include <algorithm>
#include <numbers>
#include <stdexcept>

ItemEntityManager::ItemEntityManager()
    : renderer_(std::make_unique<ItemEntityRenderer>()),
      random_(187U)
{
    const mc::content::ContentCatalog* content =
        mc::content::ContentCatalog::active();
    if (content == nullptr ||
        content->entityTypes().find(ItemEntity::typeId()) == nullptr)
    {
        throw std::runtime_error("Dropped-item entity type is not registered");
    }
}

ItemEntityManager::~ItemEntityManager() = default;

void ItemEntityManager::spawnBlockDrops(
    BlockType block,
    std::uint8_t metadata,
    const ToolProperties& tool,
    const glm::ivec3& blockPosition)
{
    for (ItemStack& drop : getBlockDrops(
             block, metadata, tool, random_))
    {
        spawnDropStack(drop, blockPosition);
    }
}

void ItemEntityManager::spawnDropStack(
    ItemStack stack,
    const glm::ivec3& blockPosition)
{
    if (stack.empty())
        return;

    // Beta dropBlockAsItem_do places the entity inside the block using a
    // 0.7-wide random region with a 0.15 inset from each side.
    const glm::vec3 position{
        static_cast<float>(blockPosition.x) +
            randomFloat(0.15f, 0.85f),
        static_cast<float>(blockPosition.y) +
            randomFloat(0.15f, 0.85f),
        static_cast<float>(blockPosition.z) +
            randomFloat(0.15f, 0.85f)
    };

    const float motionX = randomFloat(-0.1f, 0.1f);
    const float motionZ = randomFloat(-0.1f, 0.1f);
    const float hoverStart = randomFloat(
        0.0f,
        std::numbers::pi_v<float> * 2.0f
    );

    entities_.emplace_back(
        position,
        stack,
        glm::vec3(motionX, 0.2f, motionZ),
        hoverStart,
        10
    );
}

void ItemEntityManager::spawnPlayerDrop(
    ItemStack stack,
    const glm::vec3& eyePosition,
    const glm::vec3& lookDirection)
{
    if (stack.empty())
        return;

    glm::vec3 forward = lookDirection;
    if (glm::dot(forward, forward) > 0.000001f)
        forward = glm::normalize(forward);

    // Beta throws a manually dropped stack from the player rather than using
    // the random block-drop motion. Keep it moving forward and slightly up,
    // with a longer pickup delay so Q does not instantly return the item.
    const glm::vec3 position =
        eyePosition + forward * 0.35f + glm::vec3(0.0f, -0.3f, 0.0f);
    const glm::vec3 velocity =
        forward * 0.3f + glm::vec3(0.0f, 0.1f, 0.0f);

    entities_.emplace_back(
        position,
        stack,
        velocity,
        randomFloat(0.0f, std::numbers::pi_v<float> * 2.0f),
        40
    );
}

void ItemEntityManager::spawnMobDrop(
    ItemStack stack,
    const glm::vec3& position)
{
    if (stack.empty())
        return;
    entities_.emplace_back(
        position + glm::vec3(0.0f, 0.25f, 0.0f),
        stack,
        glm::vec3(
            randomFloat(-0.1f, 0.1f),
            0.2f,
            randomFloat(-0.1f, 0.1f)
        ),
        randomFloat(0.0f, std::numbers::pi_v<float> * 2.0f),
        10
    );
}

void ItemEntityManager::spawnContainerDrops(
    std::span<const ItemStack> contents,
    const glm::ivec3& blockPosition)
{
    for (const ItemStack& stored : contents)
    {
        ItemStack remaining = stored;
        while (!remaining.empty())
        {
            const int split = std::min<int>(
                remaining.count,
                std::uniform_int_distribution<int>(10, 30)(random_)
            );
            spawnDropStack(
                {remaining.item, static_cast<std::uint8_t>(split), remaining.damage},
                blockPosition
            );
            remaining.count = static_cast<std::uint8_t>(remaining.count - split);
            if (remaining.count == 0)
                remaining.clear();
        }
    }
}

void ItemEntityManager::tick(
    const World& world,
    const Player& player,
    Inventory& inventory)
{
    for (ItemEntity& entity : entities_)
    {
        entity.tick(world);

        if (entity.isDead() ||
            !entity.canBePickedUp() ||
            !entity.isNear(player.getPosition()))
        {
            continue;
        }

        ItemStack& stack = entity.getStack();
        inventory.addStack(stack);

        if (stack.empty())
            entity.kill();
    }

    std::erase_if(
        entities_,
        [](const ItemEntity& entity)
        {
            return entity.isDead();
        }
    );
}

void ItemEntityManager::draw(
    float partialTick,
    const glm::mat4& view,
    const glm::mat4& projection,
    const Texture2D& blockAtlas,
    const ItemAtlas& itemAtlas,
    const AtmosphereState& atmosphere)
{
    for (const ItemEntity& entity : entities_)
    {
        renderer_->draw(
            entity,
            partialTick,
            view,
            projection,
            blockAtlas,
            itemAtlas,
            atmosphere
        );
    }
}

std::size_t ItemEntityManager::size() const noexcept
{
    return entities_.size();
}

void ItemEntityManager::clear() noexcept
{
    entities_.clear();
}

float ItemEntityManager::randomFloat(
    float minimum,
    float maximum)
{
    std::uniform_real_distribution<float> distribution(
        minimum,
        maximum
    );
    return distribution(random_);
}
