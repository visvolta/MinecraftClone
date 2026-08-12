#include "World.h"

#include "AssetPaths.h"
#include "BlockShape.h"
#include "content/ContentCatalog.h"
#include "content/resources/ResourcePack.h"
#include "entity/Entity.h"
#include "entity/Mob.h"
#include "entity/PlayerEntity.h"
#include "entity/item/ItemEntityEntity.h"
#include "entity/item/XpOrbEntity.h"
#include "entity/spawn/WorldEntitySpawner.h"

#include <algorithm>
#include <cmath>
#include <random>

void World::setPlayer(mc::entity::PlayerEntity* player) noexcept
{
    player_ = player;
}

mc::entity::PlayerEntity* World::getPlayer() const noexcept
{
    return player_;
}

mc::entity::Entity* World::spawnEntity(
    std::unique_ptr<mc::entity::Entity> entity)
{
    if (!entity)
        return nullptr;
    mc::entity::Entity* raw = entity.get();
    entities_.push_back(std::move(entity));
    return raw;
}

void World::tickEntities()
{
    if (player_)
    {
        for (auto& entity : entities_)
        {
            if (entity && !entity->isDead())
                entity->onCollideWithPlayer(*player_);
        }
    }
    for (auto& entity : entities_)
    {
        if (!entity || entity->isDead())
            continue;
        entity->onUpdate();
    }
    std::erase_if(entities_, [](const std::unique_ptr<mc::entity::Entity>& e)
    {
        return !e || e->isDead();
    });
}

std::vector<mc::entity::Entity*> World::getEntitiesInAABB(
    const mc::entity::AxisAlignedBB& box,
    const mc::entity::Entity* exclude) const
{
    std::vector<mc::entity::Entity*> result;
    if (player_ && player_ != exclude &&
        player_->getEntityBoundingBox().intersects(box))
        result.push_back(player_);
    for (const auto& entity : entities_)
    {
        if (!entity || entity.get() == exclude || entity->isDead())
            continue;
        if (entity->getEntityBoundingBox().intersects(box))
            result.push_back(entity.get());
    }
    return result;
}

std::vector<mc::entity::Mob*> World::getMobs() const
{
    std::vector<mc::entity::Mob*> result;
    for (const auto& entity : entities_)
    {
        if (auto* mob = dynamic_cast<mc::entity::Mob*>(entity.get()))
            if (!mob->isDead())
                result.push_back(mob);
    }
    return result;
}

std::vector<mc::entity::Entity*> World::getEntities() const
{
    std::vector<mc::entity::Entity*> result;
    result.reserve(entities_.size());
    for (const auto& entity : entities_)
        if (entity)
            result.push_back(entity.get());
    return result;
}

mc::entity::PlayerEntity* World::getClosestPlayer(
    double x, double y, double z, double maxDistance) const
{
    if (!player_ || !player_->isAlive())
        return nullptr;
    const double dist = player_->getDistanceSq(x, y, z);
    if (maxDistance < 0.0 || dist <= maxDistance * maxDistance)
        return player_;
    return nullptr;
}

bool World::isAnyPlayerWithinRangeAt(
    double x, double y, double z, double range) const
{
    return getClosestPlayer(x, y, z, range) != nullptr;
}

std::vector<mc::entity::AxisAlignedBB> World::getCollisionBoxes(
    const mc::entity::Entity*,
    const mc::entity::AxisAlignedBB& area) const
{
    std::vector<mc::entity::AxisAlignedBB> boxes;
    const int minX = static_cast<int>(std::floor(area.minX)) - 1;
    const int maxX = static_cast<int>(std::floor(area.maxX)) + 1;
    const int minY = std::max(0, static_cast<int>(std::floor(area.minY)) - 1);
    const int maxY = std::min(255, static_cast<int>(std::floor(area.maxY)) + 1);
    const int minZ = static_cast<int>(std::floor(area.minZ)) - 1;
    const int maxZ = static_cast<int>(std::floor(area.maxZ)) + 1;
    for (int y = minY; y <= maxY; ++y)
    for (int z = minZ; z <= maxZ; ++z)
    for (int x = minX; x <= maxX; ++x)
    {
        if (!isBlockLoaded(x, y, z))
            continue;
        const auto state = getActualBlockState(x, y, z);
        for (const BlockBox& box : getBlockShape(state).collisionBoxes)
        {
            mc::entity::AxisAlignedBB worldBox{
                static_cast<double>(x) + box.minimum.x,
                static_cast<double>(y) + box.minimum.y,
                static_cast<double>(z) + box.minimum.z,
                static_cast<double>(x) + box.maximum.x,
                static_cast<double>(y) + box.maximum.y,
                static_cast<double>(z) + box.maximum.z
            };
            if (worldBox.intersects(area))
                boxes.push_back(worldBox);
        }
    }
    return boxes;
}

bool World::canSeeSky(int x, int y, int z) const
{
    return getSkyLightLevel(x, y, z) >= 15;
}

float World::getLightBrightness(int x, int y, int z) const
{
    const int sky = getSkyLightLevel(x, y, z);
    const int block = getBlockLightLevel(x, y, z);
    return static_cast<float>(std::max(sky, block)) / 15.0f;
}

bool World::isDaytime() const
{
    return (worldTime_ % 24000U) < 12000U;
}

mc::entity::Difficulty World::getDifficulty() const noexcept
{
    return difficulty_;
}

void World::setDifficulty(mc::entity::Difficulty difficulty) noexcept
{
    difficulty_ = difficulty;
}

std::uint64_t World::getWorldTime() const noexcept
{
    return worldTime_;
}

void World::setWorldTime(std::uint64_t time) noexcept
{
    worldTime_ = time;
}

void World::spawnXpOrbs(double x, double y, double z, int amount)
{
    while (amount > 0)
    {
        int split = amount;
        if (split > 2477) split = 2477;
        else if (split > 1237) split = 1237;
        else if (split > 617) split = 617;
        else if (split > 307) split = 307;
        else if (split > 149) split = 149;
        else if (split > 73) split = 73;
        else if (split > 37) split = 37;
        else if (split > 17) split = 17;
        else if (split > 7) split = 7;
        else if (split > 3) split = 3;
        else split = 1;
        amount -= split;
        spawnEntity(std::make_unique<mc::entity::XpOrbEntity>(*this, x, y, z, split));
    }
}

void World::dropLootTable(
    const mc::core::ResourceLocation& table,
    double x,
    double y,
    double z,
    bool killedByPlayer)
{
    try
    {
        const mc::content::resources::ResourcePack pack(AssetPaths::root());
        std::mt19937 random(static_cast<std::uint32_t>(entityRandom_.nextInt()));
        const mc::content::ContentCatalog* catalog =
            mc::content::ContentCatalog::active();
        for (const auto& drop : pack.rollLootTable(
                 table,
                 {.killedByPlayer = killedByPlayer,
                  .onFire = false,
                  .lootingLevel = 0,
                  .luck = 0.0f},
                 random))
        {
            if (!catalog)
                continue;
            if (const std::optional<ItemType> item = catalog->itemType(drop.item))
            {
                spawnItemStack(
                    ItemStack(*item, static_cast<std::uint8_t>(
                        std::clamp(drop.count, 1, 64))),
                    x, y, z);
            }
        }
    }
    catch (...)
    {
    }
}

void World::spawnItemStack(
    const ItemStack& stack,
    double x,
    double y,
    double z,
    const glm::vec3& velocity)
{
    if (stack.empty())
        return;
    auto item = std::make_unique<mc::entity::ItemEntityEntity>(*this, x, y, z, stack);
    item->motionX = velocity.x;
    item->motionY = velocity.y;
    item->motionZ = velocity.z;
    spawnEntity(std::move(item));
}

int World::countMobs(mc::entity::EnumCreatureType type) const
{
    int count = 0;
    for (mc::entity::Mob* mob : getMobs())
        if (mob->getCreatureType() == type)
            ++count;
    return count;
}

JavaRandom& World::entityRandom() noexcept
{
    return entityRandom_;
}

void World::restoreEntities(
    std::vector<std::unique_ptr<mc::entity::Entity>> entities)
{
    entities_ = std::move(entities);
}

std::vector<mc::entity::Entity*> World::takePersistentEntities()
{
    return getEntities();
}
