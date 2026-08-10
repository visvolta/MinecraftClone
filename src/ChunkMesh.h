#ifndef CHUNK_MESH_H
#define CHUNK_MESH_H

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

#include "BiomeColorMap.h"
#include "Chunk.h"
#include "ChunkVertex.h"
#include "Mesh.h"

class ChunkMeshSnapshot
{
public:
    static constexpr int Border = 1;
    static constexpr int Width = Chunk::WIDTH + Border * 2;
    static constexpr int Depth = Chunk::DEPTH + Border * 2;
    static constexpr std::size_t SampleCount =
        static_cast<std::size_t>(Width * Chunk::HEIGHT * Depth);

    int chunkX = 0;
    int chunkZ = 0;
    std::array<mc::content::BlockState, SampleCount> states{};
    std::array<std::uint8_t, SampleCount> skyLight{};
    std::array<std::uint8_t, SampleCount> blockLight{};
    std::array<float, Chunk::COLUMN_COUNT> temperatures{};
    std::array<float, Chunk::COLUMN_COUNT> humidities{};
    std::array<BiomeId, Chunk::COLUMN_COUNT> biomeIds{};
    std::array<bool, Chunk::SECTION_COUNT> emptySections{};

    [[nodiscard]] mc::content::BlockState getBlockState(
        int x, int y, int z) const noexcept;
    [[nodiscard]] std::uint8_t getSkyLight(
        int x, int y, int z) const noexcept;
    [[nodiscard]] std::uint8_t getBlockLight(
        int x, int y, int z) const noexcept;
    [[nodiscard]] float getTemperature(int x, int z) const noexcept;
    [[nodiscard]] float getHumidity(int x, int z) const noexcept;
    [[nodiscard]] BiomeId getBiome(int x, int z) const noexcept;
    [[nodiscard]] bool isSectionEmpty(int section) const noexcept;
    [[nodiscard]] int getWorldOriginX() const noexcept;
    [[nodiscard]] int getWorldOriginZ() const noexcept;

    [[nodiscard]] static constexpr std::size_t index(
        int x, int y, int z) noexcept
    {
        return static_cast<std::size_t>(
            (x + Border) + Width * ((z + Border) + Depth * y)
        );
    }
};

struct ChunkMeshInput
{
    // Meshing reads the centre chunk plus one block around its X/Z edges.
    // Store exactly that padded region instead of copying nine full chunks.
    std::unique_ptr<ChunkMeshSnapshot> snapshot;

    std::uint64_t version = 0;
    bool smoothLighting = true;
    bool fastLeaves = false;

    ChunkMeshInput() = default;
    ChunkMeshInput(const ChunkMeshInput&) = delete;
    ChunkMeshInput& operator=(const ChunkMeshInput&) = delete;
    ChunkMeshInput(ChunkMeshInput&&) noexcept = default;
    ChunkMeshInput& operator=(ChunkMeshInput&&) noexcept = default;
};

struct ChunkMeshSectionRange
{
    std::uint32_t firstIndex = 0;
    std::uint32_t indexCount = 0;
};

using ChunkMeshSectionRanges =
    std::array<ChunkMeshSectionRange, Chunk::SECTION_COUNT>;

struct ChunkMeshData
{
    int chunkX = 0;
    int chunkZ = 0;
    std::uint64_t version = 0;

    std::vector<ChunkVertex> opaqueVertices;
    std::vector<std::uint32_t> opaqueIndices;
    ChunkMeshSectionRanges opaqueRanges{};

    std::vector<ChunkVertex> cutoutVertices;
    std::vector<std::uint32_t> cutoutIndices;
    ChunkMeshSectionRanges cutoutRanges{};

    // Leaves have different topology in fast and fancy modes. Only the active
    // topology is built; the previous mesh remains as a temporary fallback
    // while an option-change rebuild is in flight.
    std::vector<ChunkVertex> fastLeafVertices;
    std::vector<std::uint32_t> fastLeafIndices;
    ChunkMeshSectionRanges fastLeafRanges{};
    std::vector<ChunkVertex> fancyLeafVertices;
    std::vector<std::uint32_t> fancyLeafIndices;
    ChunkMeshSectionRanges fancyLeafRanges{};

    // Lava is visually opaque and must write depth. Water remains a sorted,
    // blended pass, so sharing one translucent mesh causes x-ray artifacts.
    std::vector<ChunkVertex> lavaVertices;
    std::vector<std::uint32_t> lavaIndices;
    ChunkMeshSectionRanges lavaRanges{};
    std::vector<ChunkVertex> waterVertices;
    std::vector<std::uint32_t> waterIndices;
    ChunkMeshSectionRanges waterRanges{};

    int visibleFaceCount = 0;
    int vertexCount = 0;
    double cpuBuildMilliseconds = 0.0;
};

class ChunkMesh
{
public:
    ChunkMesh() = default;
    explicit ChunkMesh(ChunkMeshData data);

    static ChunkMeshData buildCpu(
        const ChunkMeshInput& input,
        const BiomeColorMap& colours
    );

    void upload(ChunkMeshData data);
    void drawOpaque(std::uint16_t sectionMask) const;
    void drawCutout(std::uint16_t sectionMask) const;
    void drawFastLeaves(std::uint16_t sectionMask) const;
    void drawFancyLeaves(std::uint16_t sectionMask) const;
    void drawLava(std::uint16_t sectionMask) const;
    void drawWater(std::uint16_t sectionMask) const;

    [[nodiscard]] int getVisibleFaceCount() const;
    [[nodiscard]] int getVertexCount() const;
    [[nodiscard]] int getChunkX() const;
    [[nodiscard]] int getChunkZ() const;
    [[nodiscard]] bool hasGeometry() const noexcept;
    [[nodiscard]] bool hasOpaque() const noexcept;
    [[nodiscard]] bool hasCutout() const noexcept;
    [[nodiscard]] bool hasLeaves(bool fast) const noexcept;
    [[nodiscard]] bool hasLava() const noexcept;
    [[nodiscard]] bool hasWater() const noexcept;

private:
    std::unique_ptr<Mesh> opaqueMesh;
    std::unique_ptr<Mesh> cutoutMesh;
    std::unique_ptr<Mesh> fastLeafMesh;
    std::unique_ptr<Mesh> fancyLeafMesh;
    std::unique_ptr<Mesh> lavaMesh;
    std::unique_ptr<Mesh> waterMesh;
    ChunkMeshSectionRanges opaqueRanges_{};
    ChunkMeshSectionRanges cutoutRanges_{};
    ChunkMeshSectionRanges fastLeafRanges_{};
    ChunkMeshSectionRanges fancyLeafRanges_{};
    ChunkMeshSectionRanges lavaRanges_{};
    ChunkMeshSectionRanges waterRanges_{};
    int visibleFaceCount = 0;
    int vertexCount = 0;
    int chunkX = 0;
    int chunkZ = 0;
};
#endif
