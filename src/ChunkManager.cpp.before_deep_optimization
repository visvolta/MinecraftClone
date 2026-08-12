#include "ChunkManager.h"

#include <utility>

Chunk& ChunkManager::getOrCreateChunk(int chunkX, int chunkZ)
{
    const ChunkKey key = makeKey(chunkX, chunkZ);

    if (const auto existing = chunks_.find(key); existing != chunks_.end())
    {
        return *existing->second;
    }

    auto chunk = std::make_unique<Chunk>(chunkX, chunkZ);
    Chunk& chunkReference = *chunk;
    chunks_.emplace(key, std::move(chunk));
    return chunkReference;
}

void ChunkManager::insertChunk(std::unique_ptr<Chunk> chunk)
{
    if (!chunk)
    {
        return;
    }

    const ChunkKey key = makeKey(chunk->getChunkX(), chunk->getChunkZ());
    chunks_.insert_or_assign(key, std::move(chunk));
}

Chunk* ChunkManager::getChunk(int chunkX, int chunkZ)
{
    const auto found = chunks_.find(makeKey(chunkX, chunkZ));
    return found != chunks_.end() ? found->second.get() : nullptr;
}

const Chunk* ChunkManager::getChunk(int chunkX, int chunkZ) const
{
    const auto found = chunks_.find(makeKey(chunkX, chunkZ));
    return found != chunks_.end() ? found->second.get() : nullptr;
}

bool ChunkManager::hasChunk(int chunkX, int chunkZ) const
{
    return chunks_.contains(makeKey(chunkX, chunkZ));
}

bool ChunkManager::removeChunk(int chunkX, int chunkZ)
{
    return chunks_.erase(makeKey(chunkX, chunkZ)) != 0;
}

void ChunkManager::clear()
{
    chunks_.clear();
}

BlockType ChunkManager::getBlockWorld(int worldX, int worldY, int worldZ) const
{
    return getBlockStateWorld(worldX, worldY, worldZ).block();
}

mc::content::BlockState ChunkManager::getBlockStateWorld(
    int worldX,
    int worldY,
    int worldZ) const
{
    if (worldY < 0 || worldY >= Chunk::HEIGHT)
    {
        return {};
    }

    const int chunkX = floorDivide(worldX, Chunk::WIDTH);
    const int chunkZ = floorDivide(worldZ, Chunk::DEPTH);

    const Chunk* chunk = getChunk(chunkX, chunkZ);
    if (chunk == nullptr)
    {
        return {};
    }

    const int localX = positiveModulo(worldX, Chunk::WIDTH);
    const int localZ = positiveModulo(worldZ, Chunk::DEPTH);
    return chunk->getBlockState(localX, worldY, localZ);
}

bool ChunkManager::setBlockWorld(
    int worldX,
    int worldY,
    int worldZ,
    BlockType block)
{
    return setBlockAndMetadataWorld(
        worldX,
        worldY,
        worldZ,
        block,
        0
    );
}

std::uint8_t ChunkManager::getMetadataWorld(
    int worldX,
    int worldY,
    int worldZ) const
{
    return static_cast<std::uint8_t>(
        getBlockStateWorld(worldX, worldY, worldZ).properties()
    );
}

bool ChunkManager::setBlockAndMetadataWorld(
    int worldX,
    int worldY,
    int worldZ,
    BlockType block,
    std::uint8_t metadata)
{
    return setBlockStateWorld(
        worldX,
        worldY,
        worldZ,
        mc::content::BlockState(block, metadata)
    );
}

bool ChunkManager::setBlockStateWorld(
    int worldX,
    int worldY,
    int worldZ,
    mc::content::BlockState state)
{
    if (worldY < 0 || worldY >= Chunk::HEIGHT)
    {
        return false;
    }

    const int chunkX = floorDivide(worldX, Chunk::WIDTH);
    const int chunkZ = floorDivide(worldZ, Chunk::DEPTH);

    Chunk* chunk = getChunk(chunkX, chunkZ);
    if (chunk == nullptr)
    {
        return false;
    }

    const int localX = positiveModulo(worldX, Chunk::WIDTH);
    const int localZ = positiveModulo(worldZ, Chunk::DEPTH);
    return chunk->setBlockState(
        localX,
        worldY,
        localZ,
        state
    );
}

std::vector<Chunk*> ChunkManager::getChunks()
{
    std::vector<Chunk*> result;
    result.reserve(chunks_.size());

    for (auto& entry : chunks_)
    {
        result.push_back(entry.second.get());
    }

    return result;
}

std::vector<const Chunk*> ChunkManager::getChunks() const
{
    std::vector<const Chunk*> result;
    result.reserve(chunks_.size());

    for (const auto& entry : chunks_)
    {
        result.push_back(entry.second.get());
    }

    return result;
}

std::size_t ChunkManager::getChunkCount() const noexcept
{
    return chunks_.size();
}

ChunkManager::ChunkKey ChunkManager::makeKey(int chunkX, int chunkZ) noexcept
{
    const auto x = static_cast<std::uint32_t>(chunkX);
    const auto z = static_cast<std::uint32_t>(chunkZ);

    return (static_cast<ChunkKey>(x) << 32U) |
           static_cast<ChunkKey>(z);
}

int ChunkManager::floorDivide(int value, int divisor) noexcept
{
    int quotient = value / divisor;
    const int remainder = value % divisor;

    if (remainder != 0 && ((remainder < 0) != (divisor < 0)))
    {
        --quotient;
    }

    return quotient;
}

int ChunkManager::positiveModulo(int value, int divisor) noexcept
{
    const int remainder = value % divisor;
    return remainder < 0 ? remainder + divisor : remainder;
}
