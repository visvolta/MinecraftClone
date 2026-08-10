#pragma once

#include "ChunkManager.h"

#include <cstddef>
#include <cstdint>
#include <deque>
#include <unordered_set>
#include <vector>

class LightingSystem
{
public:
    struct UpdatedChunk
    {
        int x = 0;
        int z = 0;
        bool highPriority = false;
        bool changed = false;
        std::uint8_t changedBorders = 0;
    };

    enum ChangedBorder : std::uint8_t
    {
        West = 1U << 0U,
        East = 1U << 1U,
        North = 1U << 2U,
        South = 1U << 3U,
        NorthWest = 1U << 4U,
        NorthEast = 1U << 5U,
        SouthWest = 1U << 6U,
        SouthEast = 1U << 7U
    };

    void queueChunk(int chunkX, int chunkZ);
    void queueChunkHighPriority(int chunkX, int chunkZ);
    void queueChunkAndNeighbours(int chunkX, int chunkZ);
    void queueChunkAndNeighboursHighPriority(int chunkX, int chunkZ);

    [[nodiscard]] std::vector<UpdatedChunk> process(
        ChunkManager& chunks,
        double budgetMilliseconds
    );

    [[nodiscard]] std::size_t getPendingCount() const noexcept;
    [[nodiscard]] double getLastUpdateMilliseconds() const noexcept;

    [[nodiscard]] static int opacity(BlockType block) noexcept;
    [[nodiscard]] static int emission(BlockType block) noexcept;

private:
    using Key = std::uint64_t;

    std::deque<UpdatedChunk> queue_;
    std::unordered_set<Key> queued_;
    double lastUpdateMilliseconds_ = 0.0;

    void queueChunkInternal(int chunkX, int chunkZ, bool highPriority);
    static Key key(int chunkX, int chunkZ) noexcept;
    [[nodiscard]] static UpdatedChunk rebuildChunkLighting(
        Chunk& chunk,
        const ChunkManager& chunks
    );
};
