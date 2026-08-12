#ifndef CHUNK_H
#define CHUNK_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>
#include "Block.h"
#include "content/BlockState.h"
#include "world/PalettedBlockStorage.h"
#include "worldgen/Biome.h"

struct ChunkSnapshot
{
    int chunkX = 0;
    int chunkZ = 0;
    std::vector<mc::content::BlockState> palette;
    std::vector<std::uint16_t> paletteIndices;
    std::array<float, 16 * 16> temperatures{};
    std::array<float, 16 * 16> humidities{};
    std::array<BiomeId, 16 * 16> biomeIds{};
    std::array<std::uint16_t, 16 * 16> worldSurfaceHeight{};
    std::array<std::uint16_t, 16 * 16> motionBlockingHeight{};
};

class Chunk
{
public:
    static constexpr int WIDTH = 16;
    static constexpr int HEIGHT = 256;
    static constexpr int DEPTH = 16;
    static constexpr int SECTION_HEIGHT = 16;
    static constexpr int SECTION_COUNT = HEIGHT / SECTION_HEIGHT;
    static constexpr std::size_t SECTION_BLOCK_COUNT =
        static_cast<std::size_t>(WIDTH * SECTION_HEIGHT * DEPTH);
    static constexpr std::size_t BLOCK_COUNT = static_cast<std::size_t>(WIDTH * HEIGHT * DEPTH);
    static constexpr std::size_t COLUMN_COUNT = static_cast<std::size_t>(WIDTH * DEPTH);

    explicit Chunk(int chunkX = 0, int chunkZ = 0);
    [[nodiscard]] mc::content::BlockState getBlockState(
        int x,
        int y,
        int z
    ) const;
    bool setBlockState(
        int x,
        int y,
        int z,
        mc::content::BlockState state
    );
    void beginBulkBlockUpdate() noexcept;
    void endBulkBlockUpdate();
    [[nodiscard]] BlockType getBlock(int x, int y, int z) const;
    bool setBlock(int x, int y, int z, BlockType block);
    bool setBlockAndMetadata(
        int x,
        int y,
        int z,
        BlockType block,
        std::uint8_t metadata
    );
    [[nodiscard]] std::uint8_t getMetadata(int x, int y, int z) const;
    bool setMetadata(int x, int y, int z, std::uint8_t metadata);
    void fill(BlockType block);
    void clear();

    void setClimate(int x, int z, float temperature, float humidity);
    [[nodiscard]] float getTemperature(int x, int z) const;
    [[nodiscard]] float getHumidity(int x, int z) const;
    void setBiome(int x, int z, BiomeId biome);
    void setBiomeAndClimate(
        int x, int z, BiomeId biome, float temperature, float humidity
    );
    [[nodiscard]] BiomeId getBiome(int x, int z) const;
    [[nodiscard]] int getWorldSurfaceHeight(int x, int z) const;
    [[nodiscard]] int getMotionBlockingHeight(int x, int z) const;
    [[nodiscard]] bool isSectionEmpty(int sectionY) const;
    [[nodiscard]] bool containsBlock(BlockType block) const noexcept;
    [[nodiscard]] bool containsBlockEntityState() const noexcept;

    [[nodiscard]] std::uint8_t getSkyLight(int x, int y, int z) const;
    [[nodiscard]] std::uint8_t getBlockLight(int x, int y, int z) const;
    void setSkyLight(int x, int y, int z, std::uint8_t level);
    void setBlockLight(int x, int y, int z, std::uint8_t level);
    void clearLighting();
    void fillSkyLight(std::uint8_t level) noexcept;
    void fillBlockLight(std::uint8_t level) noexcept;
    [[nodiscard]] const std::array<std::uint8_t, BLOCK_COUNT>&
        skyLightData() const noexcept;
    [[nodiscard]] const std::array<std::uint8_t, BLOCK_COUNT>&
        blockLightData() const noexcept;

    [[nodiscard]] int getChunkX() const;
    [[nodiscard]] int getChunkZ() const;
    [[nodiscard]] int getWorldOriginX() const;
    [[nodiscard]] int getWorldOriginZ() const;
    [[nodiscard]] ChunkSnapshot snapshot() const;
    void restore(const ChunkSnapshot& snapshot);

    [[nodiscard]] static constexpr bool inBounds(int x, int y, int z)
    { return x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT && z >= 0 && z < DEPTH; }

private:
    int chunkX = 0;
    int chunkZ = 0;
    std::array<
        mc::world::PalettedBlockStorage<SECTION_BLOCK_COUNT>,
        SECTION_COUNT
    > sections_;
    std::array<float, COLUMN_COUNT> temperatures{};
    std::array<float, COLUMN_COUNT> humidities{};
    std::array<BiomeId, COLUMN_COUNT> biomeIds{};
    std::array<std::uint16_t, COLUMN_COUNT> worldSurfaceHeight{};
    std::array<std::uint16_t, COLUMN_COUNT> motionBlockingHeight{};
    std::array<std::uint16_t, SECTION_COUNT> nonAirPerSection{};
    std::array<std::uint8_t, BLOCK_COUNT> skyLight{};
    std::array<std::uint8_t, BLOCK_COUNT> blockLight{};
    bool bulkBlockUpdate_ = false;

    [[nodiscard]] static constexpr std::size_t index(int x, int y, int z)
    { return static_cast<std::size_t>(x + WIDTH * (z + DEPTH * y)); }
    [[nodiscard]] static constexpr std::size_t sectionIndex(int x, int y, int z)
    { return static_cast<std::size_t>(x + WIDTH * (z + DEPTH * (y & 15))); }
    [[nodiscard]] static constexpr std::size_t columnIndex(int x, int z)
    { return static_cast<std::size_t>(x + WIDTH * z); }
    void updateHeightmaps(
        int x,
        int y,
        int z,
        mc::content::BlockState oldState,
        mc::content::BlockState newState
    );
    void rebuildDerivedBlockData();
};
#endif
