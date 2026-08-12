#include "entity/spawn/WorldEntitySpawner.h"

#include "World.h"
#include "entity/Mob.h"
#include "entity/PlayerEntity.h"
#include "entity/spawn/MobFactory.h"
#include "worldgen/Biome.h"

#include <cmath>
#include <unordered_set>

namespace mc::entity
{
namespace
{
constexpr int MobCountDiv = 17 * 17;

struct ChunkPos
{
    int x = 0;
    int z = 0;
    friend bool operator==(const ChunkPos&, const ChunkPos&) = default;
};

struct ChunkPosHash
{
    std::size_t operator()(const ChunkPos& p) const noexcept
    {
        return static_cast<std::size_t>(p.x) * 73856093U ^
               static_cast<std::size_t>(p.z) * 19349663U;
    }
};

const std::vector<BiomeMobSpawn>& listFor(
    const BiomeDefinition& biome, EnumCreatureType type)
{
    switch (type)
    {
        case EnumCreatureType::Monster: return biome.monsterSpawns;
        case EnumCreatureType::Creature: return biome.creatureSpawns;
        case EnumCreatureType::Ambient: return biome.ambientSpawns;
        case EnumCreatureType::WaterCreature: return biome.waterCreatureSpawns;
    }
    return biome.creatureSpawns;
}

const BiomeMobSpawn* pickWeighted(
    const std::vector<BiomeMobSpawn>& list, JavaRandom& random)
{
    int total = 0;
    for (const auto& e : list)
        total += std::max(0, e.weight);
    if (total <= 0)
        return nullptr;
    int roll = random.nextInt(total);
    for (const auto& e : list)
    {
        roll -= std::max(0, e.weight);
        if (roll < 0)
            return &e;
    }
    return nullptr;
}
}

int WorldEntitySpawner::findChunksForSpawning(World& world)
{
    PlayerEntity* player = world.getPlayer();
    if (!player || !player->isAlive())
        return 0;

    const bool spawnHostile = world.getDifficulty() != Difficulty::Peaceful;
    const bool spawnPeaceful = true;
    const bool spawnAnimals = (world.getWorldTime() % 400U) == 0U;

    std::unordered_set<ChunkPos, ChunkPosHash> eligible;
    const int pcx = static_cast<int>(std::floor(player->posX / 16.0));
    const int pcz = static_cast<int>(std::floor(player->posZ / 16.0));
    for (int dx = -8; dx <= 8; ++dx)
    for (int dz = -8; dz <= 8; ++dz)
    {
        const bool edge = dx == -8 || dx == 8 || dz == -8 || dz == 8;
        if (!edge)
            eligible.insert({pcx + dx, pcz + dz});
    }
    const int chunkCount = static_cast<int>(eligible.size());
    if (chunkCount == 0)
        return 0;

    int spawned = 0;
    constexpr EnumCreatureType types[] = {
        EnumCreatureType::Monster, EnumCreatureType::Creature,
        EnumCreatureType::Ambient, EnumCreatureType::WaterCreature
    };
    for (const EnumCreatureType type : types)
    {
        if ((!isPeacefulCreature(type) && !spawnHostile) ||
            (isPeacefulCreature(type) && !spawnPeaceful) ||
            (isAnimal(type) && !spawnAnimals))
            continue;
        const int cap = maxNumberOfCreature(type) * chunkCount / MobCountDiv;
        if (world.countMobs(type) > cap)
            continue;

        for (const ChunkPos& chunk : eligible)
        {
            JavaRandom& random = world.entityRandom();
            const int bx = chunk.x * 16 + random.nextInt(16);
            const int bz = chunk.z * 16 + random.nextInt(16);
            if (!world.isBlockLoaded(bx, 64, bz))
                continue;
            const int top = std::max(1, world.getHighestSolidBlockY(bx, bz) + 1);
            const int by = random.nextInt(std::max(1, ((top + 16) / 16) * 16));
            if (world.isSolidBlock(bx, by, bz))
                continue;
            if (world.isAnyPlayerWithinRangeAt(bx + 0.5, by, bz + 0.5, 24.0))
                continue;
            if (player->getDistanceSq(bx + 0.5, by, bz + 0.5) < 576.0)
                continue;

            const BiomeDefinition* biome =
                BiomeRegistry::active().find(world.getBiomeAt(bx, bz));
            if (!biome)
                continue;
            const BiomeMobSpawn* entry = pickWeighted(listFor(*biome, type), random);
            if (!entry)
                continue;
            auto mob = createMob(entry->entity, world);
            if (!mob)
                continue;
            mob->setLocationAndAngles(
                bx + 0.5, by, bz + 0.5, random.nextFloat() * 360.0f, 0.0f);
            if (mob->getCanSpawnHere() && mob->isNotColliding())
            {
                mob->onInitialSpawn();
                if (mob->isNotColliding())
                {
                    world.spawnEntity(std::move(mob));
                    ++spawned;
                }
            }
        }
    }
    return spawned;
}
}
