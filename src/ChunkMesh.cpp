#include "ChunkMesh.h"

#include "BlockMeshBuilder.h"
#include "TextureAtlas.h"

#include <glad/gl.h>

#include <chrono>
#include <utility>

namespace
{
void updateMesh(
    std::unique_ptr<Mesh>& mesh,
    const std::vector<ChunkVertex>& vertices,
    const std::vector<std::uint32_t>& indices)
{
    if (indices.empty())
    {
        mesh.reset();
        return;
    }

    if (mesh)
        mesh->upload(vertices, indices);
    else
        mesh = std::make_unique<Mesh>(vertices, indices);
}

void drawVisibleSections(
    const std::unique_ptr<Mesh>& mesh,
    const ChunkMeshSectionRanges& ranges,
    std::uint16_t sectionMask)
{
    if (!mesh || sectionMask == 0)
        return;

    bool active = false;
    std::uint32_t first = 0;
    std::uint32_t end = 0;

    const auto flush = [&]
    {
        if (active)
            mesh->drawRange(first, end - first);
    };

    for (int section = 0;
         section < Chunk::SECTION_COUNT;
         ++section)
    {
        const bool visible =
            (sectionMask &
             (1U << static_cast<unsigned int>(section))) != 0;

        if (!visible)
        {
            flush();
            active = false;
            continue;
        }

        const ChunkMeshSectionRange& range =
            ranges[static_cast<std::size_t>(section)];

        if (range.indexCount == 0)
            continue;

        if (!active)
        {
            first = range.firstIndex;
            active = true;
        }

        end = range.firstIndex + range.indexCount;
    }

    flush();
}
}

mc::content::BlockState ChunkMeshSnapshot::getBlockState(
    int x, int y, int z) const noexcept
{
    if (y < 0 || y > copiedMaximumY)
        return {};
    return states[index(x, y, z)];
}

std::uint8_t ChunkMeshSnapshot::getSkyLight(
    int x, int y, int z) const noexcept
{
    if (y < 0)
        return 0;
    if (y > copiedMaximumY)
        return 15;
    return skyLight[index(x, y, z)];
}

std::uint8_t ChunkMeshSnapshot::getBlockLight(
    int x, int y, int z) const noexcept
{
    if (y < 0 || y > copiedMaximumY)
        return 0;
    return blockLight[index(x, y, z)];
}

float ChunkMeshSnapshot::getTemperature(
    int x, int z) const noexcept
{
    return temperatures[climateIndex(x, z)];
}

float ChunkMeshSnapshot::getHumidity(
    int x, int z) const noexcept
{
    return humidities[climateIndex(x, z)];
}

BiomeId ChunkMeshSnapshot::getBiome(
    int x, int z) const noexcept
{
    return biomeIds[climateIndex(x, z)];
}

bool ChunkMeshSnapshot::isSectionEmpty(
    int section) const noexcept
{
    return emptySections[
        static_cast<std::size_t>(section)
    ];
}

int ChunkMeshSnapshot::getWorldOriginX() const noexcept
{
    return chunkX * Chunk::WIDTH;
}

int ChunkMeshSnapshot::getWorldOriginZ() const noexcept
{
    return chunkZ * Chunk::DEPTH;
}

ChunkMesh::ChunkMesh(ChunkMeshData data)
{
    upload(std::move(data));
}

ChunkMeshData ChunkMesh::buildCpu(
    const ChunkMeshInput& input,
    const BiomeColorMap& colourMap)
{
    const auto start =
        std::chrono::steady_clock::now();

    ChunkMeshData output;
    output.version = input.version;

    if (!input.snapshot)
        return output;

    output.chunkX = input.snapshot->chunkX;
    output.chunkZ = input.snapshot->chunkZ;

    output.opaqueVertices.reserve(16'384);
    output.opaqueIndices.reserve(24'576);
    output.cutoutVertices.reserve(2'048);
    output.cutoutIndices.reserve(3'072);

    if (input.fastLeaves)
    {
        output.fastLeafVertices.reserve(4'096);
        output.fastLeafIndices.reserve(6'144);
    }
    else
    {
        output.fancyLeafVertices.reserve(8'192);
        output.fancyLeafIndices.reserve(12'288);
    }

    output.lavaVertices.reserve(2'048);
    output.lavaIndices.reserve(3'072);
    output.waterVertices.reserve(8'192);
    output.waterIndices.reserve(12'288);

    for (int section = 0;
         section < Chunk::SECTION_COUNT;
         ++section)
    {
        if (input.snapshot->isSectionEmpty(section))
            continue;

        const std::uint32_t opaqueStart =
            static_cast<std::uint32_t>(
                output.opaqueIndices.size()
            );
        const std::uint32_t cutoutStart =
            static_cast<std::uint32_t>(
                output.cutoutIndices.size()
            );
        const std::uint32_t fastLeafStart =
            static_cast<std::uint32_t>(
                output.fastLeafIndices.size()
            );
        const std::uint32_t fancyLeafStart =
            static_cast<std::uint32_t>(
                output.fancyLeafIndices.size()
            );
        const std::uint32_t lavaStart =
            static_cast<std::uint32_t>(
                output.lavaIndices.size()
            );
        const std::uint32_t waterStart =
            static_cast<std::uint32_t>(
                output.waterIndices.size()
            );

        for (int y = section * 16;
             y < section * 16 + 16;
             ++y)
        {
            for (int z = 0;
                 z < Chunk::DEPTH;
                 ++z)
            {
                for (int x = 0;
                     x < Chunk::WIDTH;
                     ++x)
                {
                    const mc::content::BlockState state =
                        input.snapshot->getBlockState(
                            x, y, z
                        );

                    if (state.isAir())
                        continue;

                    output.visibleFaceCount +=
                        appendBlockMesh(
                            output,
                            input,
                            colourMap,
                            x,
                            y,
                            z,
                            state
                        );
                }
            }
        }

        const auto finishRange =
            [section](
                ChunkMeshSectionRanges& ranges,
                const std::vector<std::uint32_t>& indices,
                std::uint32_t start)
        {
            ranges[
                static_cast<std::size_t>(section)
            ] = {
                start,
                static_cast<std::uint32_t>(
                    indices.size()
                ) - start
            };
        };

        finishRange(
            output.opaqueRanges,
            output.opaqueIndices,
            opaqueStart
        );

        finishRange(
            output.cutoutRanges,
            output.cutoutIndices,
            cutoutStart
        );

        finishRange(
            output.fastLeafRanges,
            output.fastLeafIndices,
            fastLeafStart
        );

        finishRange(
            output.fancyLeafRanges,
            output.fancyLeafIndices,
            fancyLeafStart
        );

        finishRange(
            output.lavaRanges,
            output.lavaIndices,
            lavaStart
        );

        finishRange(
            output.waterRanges,
            output.waterIndices,
            waterStart
        );
    }

    output.vertexCount =
        static_cast<int>(
            output.opaqueVertices.size() +
            output.cutoutVertices.size() +
            output.fastLeafVertices.size() +
            output.fancyLeafVertices.size() +
            output.lavaVertices.size() +
            output.waterVertices.size()
        );

    output.cpuBuildMilliseconds =
        std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() -
            start
        ).count();

    return output;
}

void ChunkMesh::upload(ChunkMeshData data)
{
    chunkX = data.chunkX;
    chunkZ = data.chunkZ;
    visibleFaceCount = data.visibleFaceCount;
    vertexCount = data.vertexCount;

    opaqueRanges_ = data.opaqueRanges;
    cutoutRanges_ = data.cutoutRanges;
    fastLeafRanges_ = data.fastLeafRanges;
    fancyLeafRanges_ = data.fancyLeafRanges;
    lavaRanges_ = data.lavaRanges;
    waterRanges_ = data.waterRanges;

    updateMesh(
        opaqueMesh,
        data.opaqueVertices,
        data.opaqueIndices
    );

    updateMesh(
        cutoutMesh,
        data.cutoutVertices,
        data.cutoutIndices
    );

    updateMesh(
        fastLeafMesh,
        data.fastLeafVertices,
        data.fastLeafIndices
    );

    updateMesh(
        fancyLeafMesh,
        data.fancyLeafVertices,
        data.fancyLeafIndices
    );

    updateMesh(
        lavaMesh,
        data.lavaVertices,
        data.lavaIndices
    );

    updateMesh(
        waterMesh,
        data.waterVertices,
        data.waterIndices
    );
}

void ChunkMesh::drawOpaque(
    std::uint16_t sectionMask) const
{
    // 1.12.2 BlockLiquid puts lava in SOLID. Drawing it here makes lava share
    // exactly the same depth-writing stage as ordinary solid terrain instead
    // of relying on a later client pass.
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    TextureAtlas::bind(0);

    drawVisibleSections(
        opaqueMesh,
        opaqueRanges_,
        sectionMask
    );

    drawVisibleSections(
        lavaMesh,
        lavaRanges_,
        sectionMask
    );
}

void ChunkMesh::drawCutout(
    std::uint16_t sectionMask) const
{
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    TextureAtlas::bind(0);

    drawVisibleSections(
        cutoutMesh,
        cutoutRanges_,
        sectionMask
    );
}

void ChunkMesh::drawFastLeaves(
    std::uint16_t sectionMask) const
{
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    TextureAtlas::bind(0);

    if (fastLeafMesh)
    {
        drawVisibleSections(
            fastLeafMesh,
            fastLeafRanges_,
            sectionMask
        );
    }
    else if (fancyLeafMesh)
    {
        drawVisibleSections(
            fancyLeafMesh,
            fancyLeafRanges_,
            sectionMask
        );
    }
}

void ChunkMesh::drawFancyLeaves(
    std::uint16_t sectionMask) const
{
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    TextureAtlas::bind(0);

    if (fancyLeafMesh)
    {
        drawVisibleSections(
            fancyLeafMesh,
            fancyLeafRanges_,
            sectionMask
        );
    }
    else if (fastLeafMesh)
    {
        drawVisibleSections(
            fastLeafMesh,
            fastLeafRanges_,
            sectionMask
        );
    }
}

void ChunkMesh::drawLava(
    std::uint16_t sectionMask) const
{
    // Kept for compatibility. The authoritative lava draw is now part of
    // drawOpaque(), matching Minecraft 1.12.2's SOLID layer.
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    TextureAtlas::bind(0);

    drawVisibleSections(
        lavaMesh,
        lavaRanges_,
        sectionMask
    );
}

void ChunkMesh::drawWater(
    std::uint16_t sectionMask) const
{
    // This mesh contains water and registry blocks whose render layer is
    // TRANSLUCENT. The caller sorts chunks back-to-front. Always restore the
    // block atlas here because item/entity/overlay renderers rebind unit 0.
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFuncSeparate(
        GL_SRC_ALPHA,
        GL_ONE_MINUS_SRC_ALPHA,
        GL_ONE,
        GL_ZERO
    );

    TextureAtlas::bind(0);

    drawVisibleSections(
        waterMesh,
        waterRanges_,
        sectionMask
    );
}

int ChunkMesh::getVisibleFaceCount() const
{
    return visibleFaceCount;
}

int ChunkMesh::getVertexCount() const
{
    return vertexCount;
}

int ChunkMesh::getChunkX() const
{
    return chunkX;
}

int ChunkMesh::getChunkZ() const
{
    return chunkZ;
}

bool ChunkMesh::hasGeometry() const noexcept
{
    return opaqueMesh ||
           cutoutMesh ||
           fastLeafMesh ||
           fancyLeafMesh ||
           lavaMesh ||
           waterMesh;
}

bool ChunkMesh::hasOpaque() const noexcept
{
    // A lava-only visible chunk must still participate in the SOLID pass.
    return opaqueMesh != nullptr ||
           lavaMesh != nullptr;
}

bool ChunkMesh::hasCutout() const noexcept
{
    return cutoutMesh != nullptr;
}

bool ChunkMesh::hasLeaves(bool fast) const noexcept
{
    return fast
        ? fastLeafMesh != nullptr ||
          fancyLeafMesh != nullptr
        : fancyLeafMesh != nullptr ||
          fastLeafMesh != nullptr;
}

bool ChunkMesh::hasLava() const noexcept
{
    return lavaMesh != nullptr;
}

bool ChunkMesh::hasWater() const noexcept
{
    return waterMesh != nullptr;
}
