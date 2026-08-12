#include "entity/MobEntityManager.h"

#include "Player.h"
#include "BlockShape.h"
#include "World.h"
#include "worldgen/Biome.h"
#include "worldgen/JavaRandom.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <string>

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
    JavaRandom& random)
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
        const int limit = random.nextInt(7);
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
            return random.nextInt(2) == 0;
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
    entities_.reserve(128);
}

bool MobEntityManager::attackNearest(
    const glm::vec3& origin,
    const glm::vec3& direction,
    float maximumDistance,
    float damage)
{
    MobEntity* nearest = nearestAlongRay(
        origin, direction, maximumDistance
    );
    if (nearest == nullptr)
        return false;
    nearest->damage(std::max(0.0f, damage), true);
    return true;
}

bool MobEntityManager::interactNearest(
    const glm::vec3& origin,
    const glm::vec3& direction,
    float maximumDistance,
    Player& player,
    ItemStack& heldStack)
{
    MobEntity* nearest = nearestAlongRay(
        origin, direction, maximumDistance
    );
    if (nearest == nullptr)
        return false;
    const bool wasSheared = nearest->sheared_;
    const bool result = nearest->interact(player, heldStack, random_);
    if (result && nearest->type_.path() == "sheep" && !wasSheared &&
        nearest->sheared_)
    {
        constexpr std::array<BlockType, 16> wool{{
            BlockType::WhiteWool, BlockType::OrangeWool,
            BlockType::MagentaWool, BlockType::LightBlueWool,
            BlockType::YellowWool, BlockType::LimeWool,
            BlockType::PinkWool, BlockType::GrayWool,
            BlockType::LightGrayWool, BlockType::CyanWool,
            BlockType::PurpleWool, BlockType::BlueWool,
            BlockType::BrownWool, BlockType::GreenWool,
            BlockType::RedWool, BlockType::BlackWool
        }};
        const int count = 1 + random_.nextInt(3);
        for (int drop = 0; drop < count; ++drop)
            interactionDrops_.push_back({
                ItemStack(wool[static_cast<std::size_t>(
                    std::clamp(nearest->variant_, 0, 15)
                )]),
                nearest->position_ + glm::vec3(0.0f, 1.0f, 0.0f)
            });
    }
    return result;
}

void MobEntityManager::tick(
    World& world,
    Player& player,
    std::uint64_t worldTime,
    ItemType playerMainHand)
{
    ++ticks_;
    const std::uint64_t timeOfDay = worldTime % 24000U;
    const bool daytime = timeOfDay < 12000U;
    std::vector<MobBirthRequest> births;
    MobTickContext context{
        world, player, std::span<MobEntity>(entities_), births, random_,
        playerMainHand, daytime
    };
    for (MobEntity& entity : entities_)
    {
        const bool wasLeashed = entity.leashed_;
        entity.tick(context);
        if (wasLeashed && !entity.leashed_)
            interactionDrops_.push_back({
                ItemStack(ItemType::Lead, 1), entity.position_
            });
    }
    for (MobBirthRequest& birth : births)
    {
        const gameplay::MobDefinition* definition =
            registries_->mobs().find(birth.child.type);
        if (definition == nullptr || entities_.size() >= 128U)
            continue;
        entities_.emplace_back(
            birth.child.type, *definition, birth.child.position,
            birth.child.yaw
        );
        entities_.back().restorePersistentState(birth.child);
    }
    if (!births.empty())
        rebindEntities();

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
            return random_.nextInt(800) == 0;
        return false;
    });
    rebindEntities();

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

    const auto nextRange = [this](float minimum, float maximum)
    {
        return minimum + random_.nextFloat() * (maximum - minimum);
    };
    const float direction = nextRange(
        0.0f, std::numbers::pi_v<float> * 2.0f
    );
    const float distance = nextRange(24.0f, 96.0f);
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
        random_.nextInt(totalWeight);
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
        y = 46 + random_.nextInt(17);
        while (y > 45 && world.getBlock(x,y,z)!=BlockType::Water)
            --y;
        if (world.getBlock(x, y, z) != BlockType::Water)
            return;
    }
    else if (definition->category != gameplay::MobCategory::Creature)
    {
        y = 1 + random_.nextInt(std::max(1, std::min(255, surfaceY)));
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

    const int minimumGroup = std::max(1, selected->minimumGroup);
    const int maximumGroup = std::max(minimumGroup, selected->maximumGroup);
    const int groupSize = minimumGroup +
        random_.nextInt(maximumGroup - minimumGroup + 1);
    for (int member = 0;
         member < groupSize && entities_.size() < 100U;
         ++member)
    {
        const int spawnX = x + random_.nextInt(9) - 4;
        const int spawnZ = z + random_.nextInt(9) - 4;
        const int spawnY = definition->spawnPlacement ==
                gameplay::SpawnPlacement::InWater
            ? y
            : definition->category == gameplay::MobCategory::Creature
                ? world.getHighestSolidBlockY(spawnX,spawnZ)+1
                : y + random_.nextInt(3) - 1;
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
            nextRange(0.0f, std::numbers::pi_v<float> * 2.0f)
        );
        MobEntity& spawned = entities_.back();
        if (selected->entity.path() == "sheep")
        {
            const int roll = random_.nextInt(100);
            spawned.variant_ = roll < 5 ? 15 : roll < 10 ? 7
                : roll < 15 ? 8 : roll < 18 ? 12
                : random_.nextInt(500) == 0 ? 6 : 0;
        }
        else if (selected->entity.path() == "horse")
        {
            spawned.variant_ = random_.nextInt(7) + random_.nextInt(5) * 7;
            if (random_.nextInt(5) == 0)
                spawned.growingAge_ = -24000;
        }
        else if (selected->entity.path() == "llama")
        {
            spawned.variant_ = random_.nextInt(4);
        }
        else if (selected->entity.path() == "parrot")
        {
            spawned.variant_ = random_.nextInt(5);
        }
        else if (selected->entity.path() == "rabbit")
        {
            const BiomeDefinition* rabbitBiome = BiomeRegistry::active().find(
                world.getBiomeAt(spawnX, spawnZ)
            );
            const int roll = random_.nextInt(100);
            if (rabbitBiome != nullptr && rabbitBiome->snowy)
                spawned.variant_ = roll < 80 ? 1 : 3;
            else if (rabbitBiome != nullptr &&
                     rabbitBiome->name.path().find("desert") !=
                         std::string::npos)
                spawned.variant_ = 4;
            else
                spawned.variant_ = roll < 50 ? 0 : roll < 90 ? 5 : 2;
            if (member > 0)
                spawned.growingAge_ = -24000;
        }
        spawned.updateVariantPresentation();
        rebindEntities();
    }
}

MobEntity* MobEntityManager::nearestAlongRay(
    const glm::vec3& origin,
    const glm::vec3& direction,
    float maximumDistance)
{
    if (maximumDistance <= 0.0f || glm::dot(direction, direction) < 0.000001f)
        return nullptr;
    const glm::vec3 rayDirection = glm::normalize(direction);
    MobEntity* nearest = nullptr;
    float nearestDistance = maximumDistance;
    for (MobEntity& entity : entities_)
    {
        if (entity.dead())
            continue;
        const float halfWidth = entity.collisionWidth() * 0.5f;
        const glm::vec3 minimum = entity.position() +
            glm::vec3(-halfWidth, 0.0f, -halfWidth);
        const glm::vec3 maximum = entity.position() + glm::vec3(
            halfWidth, entity.collisionHeight(), halfWidth
        );
        float distance = 0.0f;
        if (rayBox(origin, rayDirection, minimum, maximum,
                   nearestDistance, distance))
        {
            nearest = &entity;
            nearestDistance = distance;
        }
    }
    return nearest;
}

std::vector<MobPersistentState> MobEntityManager::persistentStates() const
{
    std::vector<MobPersistentState> result;
    result.reserve(entities_.size());
    for (const MobEntity& entity : entities_)
        if (!entity.dead())
            result.push_back(entity.persistentState());
    return result;
}

void MobEntityManager::restorePersistentStates(
    const std::vector<MobPersistentState>& states)
{
    entities_.clear();
    for (const MobPersistentState& state : states)
    {
        const gameplay::MobDefinition* definition =
            registries_->mobs().find(state.type);
        if (definition == nullptr || entities_.size() >= 128U)
            continue;
        entities_.emplace_back(
            state.type, *definition, state.position, state.yaw
        );
        entities_.back().restorePersistentState(state);
    }
    rebindEntities();
}

void MobEntityManager::rebindEntities() noexcept
{
    for (MobEntity& entity : entities_)
        entity.rebindRuntime();
}

void MobEntityManager::clear() noexcept
{
    entities_.clear();
    deathEvents_.clear();
    interactionDrops_.clear();
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
std::vector<MobInteractionDrop> MobEntityManager::takeInteractionDrops()
{
    std::vector<MobInteractionDrop> result;
    result.swap(interactionDrops_);
    return result;
}
}
