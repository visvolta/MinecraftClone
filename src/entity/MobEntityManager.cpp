#include "entity/MobEntityManager.h"

#include "Player.h"
#include "BlockShape.h"
#include "World.h"
#include "worldgen/Biome.h"
#include "worldgen/JavaRandom.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace mc::entity
{
namespace
{
bool rayBox(
    const glm::vec3& origin,
    const glm::vec3& direction,
    const glm::vec3& minimum,
    const glm::vec3& maximum,
    float maximumDistance,
    float& distance)
{
    float nearDistance = 0.0f;
    float farDistance = maximumDistance;
    for (int axis = 0; axis < 3; ++axis)
    {
        if (std::abs(direction[axis]) < 0.000001f)
        {
            if (origin[axis] < minimum[axis] || origin[axis] > maximum[axis])
                return false;
            continue;
        }
        const float inverse = 1.0f / direction[axis];
        float first = (minimum[axis] - origin[axis]) * inverse;
        float second = (maximum[axis] - origin[axis]) * inverse;
        if (first > second)
            std::swap(first, second);
        nearDistance = std::max(nearDistance, first);
        farDistance = std::min(farDistance, second);
        if (nearDistance > farDistance)
            return false;
    }
    distance = nearDistance;
    return nearDistance <= maximumDistance;
}

std::size_t categoryCap(gameplay::MobCategory category)
{
    switch (category)
    {
        case gameplay::MobCategory::Monster: return 70;
        case gameplay::MobCategory::Creature: return 10;
        case gameplay::MobCategory::Ambient: return 15;
        case gameplay::MobCategory::WaterCreature: return 5;
    }
    return 0;
}

const std::vector<BiomeMobSpawn>& spawnList(
    const BiomeDefinition& biome,
    gameplay::MobCategory category)
{
    switch (category)
    {
        case gameplay::MobCategory::Monster: return biome.monsterSpawns;
        case gameplay::MobCategory::Creature: return biome.creatureSpawns;
        case gameplay::MobCategory::Ambient: return biome.ambientSpawns;
        case gameplay::MobCategory::WaterCreature:
            return biome.waterCreatureSpawns;
    }
    return biome.creatureSpawns;
}

bool spawnSpaceClear(
    const World& world,
    const gameplay::MobDefinition& definition,
    const glm::vec3& position)
{
    const float halfWidth = definition.width * 0.5f;
    const glm::vec3 minimum = position +
        glm::vec3(-halfWidth, 0.0f, -halfWidth);
    const glm::vec3 maximum = position +
        glm::vec3(halfWidth, definition.height, halfWidth);
    for (int x = static_cast<int>(std::floor(minimum.x));
         x <= static_cast<int>(std::floor(maximum.x)); ++x)
    for (int y = static_cast<int>(std::floor(minimum.y));
         y <= static_cast<int>(std::floor(maximum.y)); ++y)
    for (int z = static_cast<int>(std::floor(minimum.z));
         z <= static_cast<int>(std::floor(maximum.z)); ++z)
    {
        if (!getBlockShape(world.getActualBlockState(x, y, z))
                 .collisionBoxes.empty())
            return false;
    }
    return true;
}

bool specialSpawnRules(
    const core::ResourceLocation& type,
    const gameplay::MobDefinition& definition,
    const World& world,
    int x,
    int y,
    int z,
    std::mt19937& random)
{
    const std::string& name = type.path();
    const BlockType ground = world.getBlock(x,y-1,z);
    if (definition.spawnPlacement == gameplay::SpawnPlacement::OnGround &&
        getBlockShape(world.getActualBlockState(x,y-1,z)).collisionBoxes.empty())
        return false;
    if (definition.category == gameplay::MobCategory::Creature)
    {
        bool validGround = ground == BlockType::Grass;
        if (name == "rabbit")
            validGround = validGround || ground == BlockType::Sand ||
                ground == BlockType::Snow;
        else if (name == "polar_bear")
            validGround = validGround || ground == BlockType::Snow ||
                ground == BlockType::Ice;
        else if (name == "mushroom_cow")
            validGround = ground == BlockType::Mycelium;
        else if (name == "parrot")
            validGround = validGround || isLeaf(ground);
        else if (name == "ocelot")
            validGround = ground == BlockType::Grass || isLeaf(ground);
        if (!validGround)
            return false;
    }
    if (name == "ocelot")
        return y >= 63 && (ground == BlockType::Grass || isLeaf(ground));
    if (name == "bat")
    {
        if (y >= 63)
            return false;
        const int limit = std::uniform_int_distribution<int>(0,6)(random);
        return world.getBlockLightLevel(x,y,z) <= limit;
    }
    if (name == "squid")
        return y > 45 && y < 63 && world.getBlock(x,y,z) == BlockType::Water;
    if (name == "guardian" || name == "elder_guardian")
        return world.getBlock(x,y,z) == BlockType::Water;
    if (name == "slime")
    {
        const BiomeId biome = world.getBiomeAt(x,z);
        if (biome == VanillaBiomes::Swampland && y > 50 && y < 70 &&
            world.getBlockLightLevel(x,y,z) <= 7)
            return std::uniform_int_distribution<int>(0,1)(random) == 0;
        if (y >= 40)
            return false;
        const std::int64_t chunkX = x >= 0 ? x/16 : (x-15)/16;
        const std::int64_t chunkZ = z >= 0 ? z/16 : (z-15)/16;
        JavaRandom slimeRandom(
            (static_cast<std::int64_t>(world.getSeed()) +
             chunkX*chunkX*4987142LL + chunkX*5947611LL +
             chunkZ*chunkZ*4392871LL + chunkZ*389711LL) ^ 987234911LL
        );
        return slimeRandom.nextInt(10) == 0;
    }
    return true;
}
}

MobEntityManager::MobEntityManager(
    const gameplay::GameplayRegistries& registries)
    : registries_(&registries)
{
}

bool MobEntityManager::attackNearest(
    const glm::vec3& origin,
    const glm::vec3& direction,
    float maximumDistance,
    float damage)
{
    if (maximumDistance <= 0.0f || glm::dot(direction, direction) < 0.000001f)
        return false;
    const glm::vec3 rayDirection = glm::normalize(direction);
    MobEntity* nearest = nullptr;
    float nearestDistance = maximumDistance;
    for (MobEntity& entity : entities_)
    {
        if (entity.dead())
            continue;
        const float halfWidth = entity.definition().width * 0.5f;
        const glm::vec3 minimum = entity.position() +
            glm::vec3(-halfWidth, 0.0f, -halfWidth);
        const glm::vec3 maximum = entity.position() + glm::vec3(
            halfWidth, entity.definition().height, halfWidth
        );
        float distance = 0.0f;
        if (rayBox(origin, rayDirection, minimum, maximum,
                   nearestDistance, distance))
        {
            nearest = &entity;
            nearestDistance = distance;
        }
    }
    if (nearest == nullptr)
        return false;
    nearest->damage(std::max(0.0f, damage), true);
    return true;
}

void MobEntityManager::tick(
    World& world,
    Player& player,
    std::uint64_t worldTime)
{
    ++ticks_;
    const std::uint64_t timeOfDay = worldTime % 24000U;
    const bool daytime = timeOfDay < 12000U;
    for (MobEntity& entity : entities_)
        entity.tick(world, player, daytime, random_);

    const glm::vec3 playerPosition = player.getPosition();
    for (const MobEntity& entity : entities_)
    {
        if (entity.dead() && entity.deathTicks() == 1)
            deathEvents_.push_back({
                entity.definition().lootTable,
                entity.position(),
                entity.killedByPlayer()
            });
    }
    std::erase_if(entities_, [&](const MobEntity& entity)
    {
        const glm::vec3 difference = entity.position() - playerPosition;
        const float distanceSquared = glm::dot(difference, difference);
        if ((entity.dead() && entity.deathTicks() >= 20) ||
            distanceSquared > 128.0f * 128.0f)
            return true;
        if (distanceSquared > 32.0f * 32.0f &&
            entity.age() > 600 && entity.age() % 800 == 0)
            return std::uniform_int_distribution<int>(0, 799)(random_) == 0;
        return false;
    });

    if (ticks_ % 5U == 0U)
    {
        const float angle = static_cast<float>(timeOfDay) / 24000.0f;
        const float darkness = 1.0f - std::clamp(
            std::cos(angle * std::numbers::pi_v<float> * 2.0f) * 2.0f + 0.5f,
            0.0f, 1.0f
        );
        const int skylightSubtracted = static_cast<int>(darkness * 11.0f);
        tryNaturalSpawn(world, player, gameplay::MobCategory::Monster,
                        skylightSubtracted);
        tryNaturalSpawn(world, player, gameplay::MobCategory::Creature,
                        skylightSubtracted);
        tryNaturalSpawn(world, player, gameplay::MobCategory::Ambient,
                        skylightSubtracted);
        tryNaturalSpawn(world, player, gameplay::MobCategory::WaterCreature,
                        skylightSubtracted);
    }
}

void MobEntityManager::tryNaturalSpawn(
    World& world,
    const Player& player,
    gameplay::MobCategory category,
    int skylightSubtracted)
{
    const std::size_t current = static_cast<std::size_t>(std::count_if(
        entities_.begin(), entities_.end(),
        [category](const MobEntity& entity)
        {
            return entity.definition().category == category;
        }
    ));
    if (current >= categoryCap(category))
        return;

    std::uniform_real_distribution<float> angle(
        0.0f, std::numbers::pi_v<float> * 2.0f
    );
    std::uniform_real_distribution<float> radius(24.0f, 96.0f);
    const float direction = angle(random_);
    const float distance = radius(random_);
    const int x = static_cast<int>(std::floor(
        player.getPosition().x + std::cos(direction) * distance
    ));
    const int z = static_cast<int>(std::floor(
        player.getPosition().z + std::sin(direction) * distance
    ));
    if (!world.isBlockLoaded(x, 64, z))
        return;

    const BiomeDefinition* biome =
        BiomeRegistry::active().find(world.getBiomeAt(x, z));
    if (biome == nullptr)
        return;
    const std::vector<BiomeMobSpawn>& choices = spawnList(*biome, category);
    int totalWeight = 0;
    for (const BiomeMobSpawn& choice : choices)
        totalWeight += std::max(0, choice.weight);
    if (totalWeight == 0)
        return;
    int selectedWeight =
        std::uniform_int_distribution<int>(0, totalWeight - 1)(random_);
    const BiomeMobSpawn* selected = nullptr;
    for (const BiomeMobSpawn& choice : choices)
    {
        selectedWeight -= std::max(0, choice.weight);
        if (selectedWeight < 0)
        {
            selected = &choice;
            break;
        }
    }
    if (selected == nullptr)
        return;
    const gameplay::MobDefinition* definition =
        registries_->mobs().find(selected->entity);
    if (definition == nullptr)
        return;

    const int surfaceY = world.getHighestSolidBlockY(x,z)+1;
    int y = surfaceY;
    if (definition->spawnPlacement == gameplay::SpawnPlacement::InWater)
    {
        y = std::uniform_int_distribution<int>(46,62)(random_);
        while (y > 45 && world.getBlock(x,y,z)!=BlockType::Water)
            --y;
        if (world.getBlock(x, y, z) != BlockType::Water)
            return;
    }
    else if (definition->category != gameplay::MobCategory::Creature)
    {
        y = std::uniform_int_distribution<int>(
            1,std::max(1,std::min(255,surfaceY))
        )(random_);
    }
    else if (world.getBlock(x, y - 1, z) == BlockType::Air)
    {
        return;
    }

    if (definition->category == gameplay::MobCategory::Monster)
    {
        const int effectiveSkyLight = std::max(
            0, world.getSkyLightLevel(x, y, z) - skylightSubtracted
        );
        if (effectiveSkyLight > 7 ||
            world.getBlockLightLevel(x, y, z) > 7)
            return;
    }
    else if (definition->category == gameplay::MobCategory::Creature &&
             world.getSkyLightLevel(x, y, z) < 9)
    {
        return;
    }

    const int groupSize = std::uniform_int_distribution<int>(
        selected->minimumGroup, selected->maximumGroup
    )(random_);
    std::uniform_int_distribution<int> offset(-4, 4);
    std::uniform_int_distribution<int> verticalOffset(-1,1);
    for (int member = 0;
         member < groupSize && entities_.size() < 100U;
         ++member)
    {
        const int spawnX = x + offset(random_);
        const int spawnZ = z + offset(random_);
        const int spawnY = definition->spawnPlacement ==
                gameplay::SpawnPlacement::InWater
            ? y
            : definition->category == gameplay::MobCategory::Creature
                ? world.getHighestSolidBlockY(spawnX,spawnZ)+1
                : y+verticalOffset(random_);
        const glm::vec3 spawnPosition(
            spawnX + 0.5f, static_cast<float>(spawnY), spawnZ + 0.5f
        );
        if (!specialSpawnRules(
                selected->entity,*definition,world,
                spawnX,spawnY,spawnZ,random_))
            continue;
        if (!spawnSpaceClear(world, *definition, spawnPosition))
            continue;
        entities_.emplace_back(
            selected->entity,
            *definition,
            spawnPosition,
            angle(random_)
        );
    }
}

void MobEntityManager::clear() noexcept
{
    entities_.clear();
    deathEvents_.clear();
}
const std::vector<MobEntity>& MobEntityManager::entities() const noexcept
{
    return entities_;
}
std::size_t MobEntityManager::size() const noexcept { return entities_.size(); }
std::vector<MobDeath> MobEntityManager::takeDeaths()
{
    std::vector<MobDeath> result;
    result.swap(deathEvents_);
    return result;
}
}
