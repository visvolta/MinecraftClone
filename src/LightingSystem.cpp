#include "LightingSystem.h"

#include "content/ContentCatalog.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <vector>

namespace
{
using Clock = std::chrono::steady_clock;

struct LightNode
{
    int x = 0;
    int y = 0;
    int z = 0;
};

constexpr std::array<std::array<int, 3>, 6> Directions{{
    {{-1,0,0}}, {{1,0,0}}, {{0,-1,0}},
    {{0,1,0}}, {{0,0,-1}}, {{0,0,1}}
}};

double elapsedMs(Clock::time_point start)
{
    return std::chrono::duration<double, std::milli>(
        Clock::now() - start
    ).count();
}
}

LightingSystem::Key LightingSystem::key(int x, int z) noexcept
{
    return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(x)) << 32U) |
           static_cast<std::uint32_t>(z);
}

void LightingSystem::queueChunkInternal(
    int chunkX,
    int chunkZ,
    bool highPriority)
{
    const Key chunkKey = key(chunkX, chunkZ);
    if (!queued_.insert(chunkKey).second)
    {
        if (highPriority)
        {
            const auto existing = std::find_if(
                queue_.begin(), queue_.end(),
                [chunkX, chunkZ](const UpdatedChunk& value)
                {
                    return value.x == chunkX && value.z == chunkZ;
                }
            );
            if (existing != queue_.end() && !existing->highPriority)
            {
                UpdatedChunk promoted = *existing;
                promoted.highPriority = true;
                queue_.erase(existing);
                queue_.push_front(promoted);
            }
        }
        return;
    }

    UpdatedChunk update;
    update.x = chunkX;
    update.z = chunkZ;
    update.highPriority = highPriority;
    if (highPriority)
        queue_.push_front(update);
    else
        queue_.push_back(update);
}

void LightingSystem::queueChunk(int chunkX, int chunkZ)
{
    queueChunkInternal(chunkX, chunkZ, false);
}

void LightingSystem::queueChunkHighPriority(int chunkX, int chunkZ)
{
    queueChunkInternal(chunkX, chunkZ, true);
}

void LightingSystem::queueChunkAndNeighbours(int chunkX, int chunkZ)
{
    queueChunkInternal(chunkX, chunkZ, false);
    queueChunkInternal(chunkX - 1, chunkZ, false);
    queueChunkInternal(chunkX + 1, chunkZ, false);
    queueChunkInternal(chunkX, chunkZ - 1, false);
    queueChunkInternal(chunkX, chunkZ + 1, false);
}

void LightingSystem::queueChunkAndNeighboursHighPriority(
    int chunkX,
    int chunkZ)
{
    // Push neighbours first because push_front reverses insertion order.
    queueChunkInternal(chunkX, chunkZ + 1, true);
    queueChunkInternal(chunkX, chunkZ - 1, true);
    queueChunkInternal(chunkX + 1, chunkZ, true);
    queueChunkInternal(chunkX - 1, chunkZ, true);
    queueChunkInternal(chunkX, chunkZ, true);
}

std::vector<LightingSystem::UpdatedChunk> LightingSystem::process(
    ChunkManager& chunks,
    double budgetMilliseconds)
{
    const auto start = Clock::now();
    std::vector<UpdatedChunk> updated;

    while (!queue_.empty() && elapsedMs(start) < budgetMilliseconds)
    {
        const UpdatedChunk coordinate = queue_.front();
        queue_.pop_front();
        queued_.erase(key(coordinate.x, coordinate.z));

        Chunk* chunk = chunks.getChunk(coordinate.x, coordinate.z);
        if (chunk == nullptr)
            continue;

        UpdatedChunk result = rebuildChunkLighting(*chunk, chunks);
        result.x = coordinate.x;
        result.z = coordinate.z;
        result.highPriority = coordinate.highPriority;
        updated.push_back(result);
    }

    lastUpdateMilliseconds_ = elapsedMs(start);
    return updated;
}

std::size_t LightingSystem::getPendingCount() const noexcept
{
    return queue_.size();
}

double LightingSystem::getLastUpdateMilliseconds() const noexcept
{
    return lastUpdateMilliseconds_;
}

int LightingSystem::opacity(BlockType block) noexcept
{
    if (const mc::content::ContentCatalog* catalog =
            mc::content::ContentCatalog::active())
    {
        if (const mc::content::BlockDefinition* definition =
                catalog->block(block))
            return definition->behaviour.lightOpacity;
    }
    if (block == BlockType::Air || isPlant(block) || isLadder(block) ||
        block == BlockType::Lava)
        return 0;
    if (block == BlockType::Water)
        return 3;
    if (isLeaf(block))
        return 1;
    return 15;
}

int LightingSystem::emission(BlockType block) noexcept
{
    if (const mc::content::ContentCatalog* catalog =
            mc::content::ContentCatalog::active())
    {
        if (const mc::content::BlockDefinition* definition =
                catalog->block(block))
            return definition->behaviour.lightEmission;
    }
    if (block == BlockType::Lava)
        return 15;
    return block == BlockType::LitFurnace ? 13 : 0;
}

LightingSystem::UpdatedChunk LightingSystem::rebuildChunkLighting(
    Chunk& chunk,
    const ChunkManager& chunks)
{
    const auto previousSky = chunk.skyLightData();
    const auto previousBlock = chunk.blockLightData();

    // Content is frozen before the world workers start. Resolve registry
    // behaviour once per relight instead of performing several hash lookups
    // for every voxel and every flood-fill edge.
    std::array<std::uint8_t, 256> opacityCache{};
    std::array<std::uint8_t, 256> emissionCache{};
    opacityCache.fill(15);
    for (int value = 0;
         value <= static_cast<int>(BlockType::TNT);
         ++value)
    {
        const BlockType block = static_cast<BlockType>(value);
        opacityCache[static_cast<std::size_t>(value)] =
            static_cast<std::uint8_t>(opacity(block));
        emissionCache[static_cast<std::size_t>(value)] =
            static_cast<std::uint8_t>(emission(block));
    }
    const auto blockOpacity = [&opacityCache](BlockType block) noexcept
    {
        return static_cast<int>(
            opacityCache[static_cast<std::uint8_t>(block)]
        );
    };
    const auto blockEmission = [&emissionCache](BlockType block) noexcept
    {
        return static_cast<int>(
            emissionCache[static_cast<std::uint8_t>(block)]
        );
    };

    chunk.clearLighting();

    // Beta-style vertical skylight: starts at 15 and is reduced by block
    // opacity while descending. Fully opaque blocks stop the column.
    for (int x = 0; x < Chunk::WIDTH; ++x)
    {
        for (int z = 0; z < Chunk::DEPTH; ++z)
        {
            int level = 15;
            for (int y = Chunk::HEIGHT - 1; y >= 0; --y)
            {
                const int attenuation = blockOpacity(
                    chunk.getBlock(x, y, z)
                );
                if (attenuation >= 15)
                    level = 0;
                else if (attenuation > 0)
                    level = std::max(0, level - attenuation);

                chunk.setSkyLight(x, y, z, static_cast<std::uint8_t>(level));
            }
        }
    }

    // Seed both lighting channels from loaded neighbouring boundaries.
    // Missing neighbours are not treated as full sunlight; doing so creates
    // bright seams that later turn into dark bands when the neighbour loads.
    for (int y = 0; y < Chunk::HEIGHT; ++y)
    {
        for (int edge = 0; edge < Chunk::WIDTH; ++edge)
        {
            const struct Seed { int x,z,nx,nz; } seeds[4] = {
                {0, edge, -1, edge},
                {Chunk::WIDTH - 1, edge, Chunk::WIDTH, edge},
                {edge, 0, edge, -1},
                {edge, Chunk::DEPTH - 1, edge, Chunk::DEPTH}
            };

            for (const Seed& seed : seeds)
            {
                const int worldX = chunk.getWorldOriginX() + seed.nx;
                const int worldZ = chunk.getWorldOriginZ() + seed.nz;
                const int neighbourChunkX =
                    worldX >= 0 ? worldX / Chunk::WIDTH
                                : (worldX - Chunk::WIDTH + 1) / Chunk::WIDTH;
                const int neighbourChunkZ =
                    worldZ >= 0 ? worldZ / Chunk::DEPTH
                                : (worldZ - Chunk::DEPTH + 1) / Chunk::DEPTH;

                const Chunk* neighbour =
                    chunks.getChunk(neighbourChunkX, neighbourChunkZ);
                if (neighbour == nullptr)
                    continue;

                const int localX = worldX - neighbour->getWorldOriginX();
                const int localZ = worldZ - neighbour->getWorldOriginZ();
                const int attenuation =
                    std::max(1, blockOpacity(
                        chunk.getBlock(seed.x, y, seed.z)
                    ));

                const int skyCandidate = std::max(
                    0,
                    static_cast<int>(neighbour->getSkyLight(localX, y, localZ)) -
                    attenuation
                );
                if (skyCandidate > chunk.getSkyLight(seed.x, y, seed.z))
                {
                    chunk.setSkyLight(
                        seed.x, y, seed.z,
                        static_cast<std::uint8_t>(skyCandidate)
                    );
                }

                const int blockCandidate = std::max(
                    0,
                    static_cast<int>(neighbour->getBlockLight(localX, y, localZ)) -
                    attenuation
                );
                if (blockCandidate > chunk.getBlockLight(seed.x, y, seed.z))
                {
                    chunk.setBlockLight(
                        seed.x, y, seed.z,
                        static_cast<std::uint8_t>(blockCandidate)
                    );
                }
            }
        }
    }

    // Flood-fill skylight and emitted/block-boundary light inside the chunk.
    std::vector<LightNode> skyQueue;
    std::vector<LightNode> blockQueue;
    skyQueue.reserve(Chunk::BLOCK_COUNT / 8U);
    blockQueue.reserve(256);

    const auto canSpreadSky = [&](int x, int y, int z)
    {
        const int currentLevel = chunk.getSkyLight(x, y, z);
        if (currentLevel <= 1)
            return false;

        // Direct vertical skylight is already complete. Only horizontal
        // frontiers can introduce light into caves and overhangs.
        constexpr std::array<std::array<int, 2>, 4> horizontal{{
            {{-1, 0}}, {{1, 0}}, {{0, -1}}, {{0, 1}}
        }};
        for (const auto& direction : horizontal)
        {
            const int nx = x + direction[0];
            const int nz = z + direction[1];
            if (!Chunk::inBounds(nx, y, nz))
                continue;
            const int attenuation =
                std::max(1, blockOpacity(chunk.getBlock(nx, y, nz)));
            const int existing = chunk.getSkyLight(nx, y, nz);
            if (currentLevel - attenuation > existing)
                return true;
        }
        return false;
    };

    for (int y = 0; y < Chunk::HEIGHT; ++y)
        for (int z = 0; z < Chunk::DEPTH; ++z)
            for (int x = 0; x < Chunk::WIDTH; ++x)
            {
                const int emitted = blockEmission(chunk.getBlock(x,y,z));
                if (emitted > chunk.getBlockLight(x,y,z))
                {
                    chunk.setBlockLight(
                        x, y, z,
                        static_cast<std::uint8_t>(emitted)
                    );
                }

                // The vertical pass already assigns most open-air skylight.
                // Seed only voxels that can actually improve a neighbour;
                // pushing every lit air voxel made one chunk relight visit
                // hundreds of thousands of redundant edges.
                if (canSpreadSky(x, y, z))
                    skyQueue.push_back({x,y,z});
                // Before propagation, block light is non-zero only at an
                // emitting block or a loaded-neighbour boundary seed.
                if (chunk.getBlockLight(x, y, z) > 1)
                    blockQueue.push_back({x,y,z});
            }

    const auto propagate = [&](std::vector<LightNode>& nodes, bool sky)
    {
        std::size_t next = 0;
        while (next < nodes.size())
        {
            const LightNode current = nodes[next++];

            const int currentLevel = sky
                ? chunk.getSkyLight(current.x,current.y,current.z)
                : chunk.getBlockLight(current.x,current.y,current.z);

            if (currentLevel <= 1)
                continue;

            for (const auto& direction : Directions)
            {
                const int nx = current.x + direction[0];
                const int ny = current.y + direction[1];
                const int nz = current.z + direction[2];
                if (!Chunk::inBounds(nx,ny,nz))
                    continue;

                const int attenuation =
                    std::max(1, blockOpacity(chunk.getBlock(nx,ny,nz)));
                const int candidate =
                    std::max(0, currentLevel - attenuation);
                const int existing = sky
                    ? chunk.getSkyLight(nx,ny,nz)
                    : chunk.getBlockLight(nx,ny,nz);

                if (candidate <= existing)
                    continue;

                if (sky)
                    chunk.setSkyLight(nx,ny,nz,
                        static_cast<std::uint8_t>(candidate));
                else
                    chunk.setBlockLight(nx,ny,nz,
                        static_cast<std::uint8_t>(candidate));

                nodes.push_back({nx,ny,nz});
            }
        }
    };

    propagate(skyQueue, true);
    propagate(blockQueue, false);

    UpdatedChunk result;
    const auto& currentSky = chunk.skyLightData();
    const auto& currentBlock = chunk.blockLightData();
    for (int y = 0; y < Chunk::HEIGHT; ++y)
    {
        for (int z = 0; z < Chunk::DEPTH; ++z)
        {
            for (int x = 0; x < Chunk::WIDTH; ++x)
            {
                const std::size_t index = static_cast<std::size_t>(
                    x + Chunk::WIDTH * (z + Chunk::DEPTH * y)
                );
                if (previousSky[index] == currentSky[index] &&
                    previousBlock[index] == currentBlock[index])
                {
                    continue;
                }

                result.changed = true;
                if (x == 0) result.changedBorders |= West;
                if (x == Chunk::WIDTH - 1) result.changedBorders |= East;
                if (z == 0) result.changedBorders |= North;
                if (z == Chunk::DEPTH - 1) result.changedBorders |= South;
                if (x == 0 && z == 0)
                    result.changedBorders |= NorthWest;
                if (x == Chunk::WIDTH - 1 && z == 0)
                    result.changedBorders |= NorthEast;
                if (x == 0 && z == Chunk::DEPTH - 1)
                    result.changedBorders |= SouthWest;
                if (x == Chunk::WIDTH - 1 && z == Chunk::DEPTH - 1)
                    result.changedBorders |= SouthEast;
            }
        }
    }
    return result;
}
