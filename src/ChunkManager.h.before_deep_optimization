#pragma once

#include "Block.h"
#include "Chunk.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

class ChunkManager
{
public:
    ChunkManager() = default;
    ~ChunkManager() = default;

    ChunkManager(const ChunkManager&) = delete;
    ChunkManager& operator=(const ChunkManager&) = delete;
    ChunkManager(ChunkManager&&) noexcept = default;
    ChunkManager& operator=(ChunkManager&&) noexcept = default;

    Chunk& getOrCreateChunk(int chunkX, int chunkZ);
    void insertChunk(std::unique_ptr<Chunk> chunk);

    [[nodiscard]] Chunk* getChunk(int chunkX, int chunkZ);
    [[nodiscard]] const Chunk* getChunk(int chunkX, int chunkZ) const;
    [[nodiscard]] bool hasChunk(int chunkX, int chunkZ) const;

    bool removeChunk(int chunkX, int chunkZ);
    void clear();

    [[nodiscard]] mc::content::BlockState getBlockStateWorld(
        int worldX,
        int worldY,
        int worldZ
    ) const;
    bool setBlockStateWorld(
        int worldX,
        int worldY,
        int worldZ,
        mc::content::BlockState state
    );
    [[nodiscard]] BlockType getBlockWorld(int worldX, int worldY, int worldZ) const;
    bool setBlockWorld(int worldX, int worldY, int worldZ, BlockType block);
    [[nodiscard]] std::uint8_t getMetadataWorld(
        int worldX,
        int worldY,
        int worldZ
    ) const;
    bool setBlockAndMetadataWorld(
        int worldX,
        int worldY,
        int worldZ,
        BlockType block,
        std::uint8_t metadata
    );

    [[nodiscard]] std::vector<Chunk*> getChunks();
    [[nodiscard]] std::vector<const Chunk*> getChunks() const;
    [[nodiscard]] std::size_t getChunkCount() const noexcept;

private:
    using ChunkKey = std::uint64_t;
    using ChunkMap = std::unordered_map<ChunkKey, std::unique_ptr<Chunk>>;

    ChunkMap chunks_;

    [[nodiscard]] static ChunkKey makeKey(int chunkX, int chunkZ) noexcept;
    [[nodiscard]] static int floorDivide(int value, int divisor) noexcept;
    [[nodiscard]] static int positiveModulo(int value, int divisor) noexcept;
};
