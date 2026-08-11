#include "FluidMeshBuilder.h"

#include "ChunkMeshing.h"
#include "FluidState.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace
{
struct FluidMaterials
{
    MaterialTexture still;
    MaterialTexture flow;
};

[[nodiscard]] constexpr FluidMaterials materialsFor(
    BlockType liquid) noexcept
{
    return liquid == BlockType::Lava
        ? FluidMaterials{MaterialTexture::LavaStill, MaterialTexture::LavaFlow}
        : FluidMaterials{MaterialTexture::WaterStill, MaterialTexture::WaterFlow};
}

float cornerHeight(
    const ChunkMeshInput& input,
    int x,
    int y,
    int z,
    BlockType liquid)
{
    int sampleWeight = 0;
    float airWeight = 0.0f;

    // Matches BlockFluid.getFluidHeightPercent/RenderBlocks: four cells feed
    // each corner and sources/falling columns receive tenfold weighting.
    for (int corner = 0; corner < 4; ++corner)
    {
        const int sampleX = x - (corner & 1);
        const int sampleZ = z - ((corner >> 1) & 1);
        if (ChunkMeshing::sampleBlock(
                input, sampleX, y + 1, sampleZ) == liquid)
        {
            return 1.0f;
        }

        const mc::content::BlockState state = ChunkMeshing::sampleState(
            input, sampleX, y, sampleZ
        );
        const BlockType block = state.block();
        if (block != liquid)
        {
            if (!isSolid(block))
            {
                airWeight += 1.0f;
                ++sampleWeight;
            }
            continue;
        }

        const std::uint8_t level = static_cast<std::uint8_t>(state.properties());
        if (FluidState::isFalling(level) ||
            level == FluidState::SourceLevel)
        {
            airWeight += FluidState::airFraction(level) * 10.0f;
            sampleWeight += 10;
        }
        airWeight += FluidState::airFraction(level);
        ++sampleWeight;
    }

    return sampleWeight > 0
        ? 1.0f - airWeight / static_cast<float>(sampleWeight)
        : 0.0f;
}

int effectiveLevel(
    const ChunkMeshInput& input,
    int x,
    int y,
    int z,
    BlockType liquid)
{
    const mc::content::BlockState state =
        ChunkMeshing::sampleState(input, x, y, z);
    if (state.block() != liquid)
        return -1;
    return FluidState::effectiveLevel(state.properties());
}

bool blocksFlow(
    const ChunkMeshInput& input,
    int x,
    int y,
    int z)
{
    const BlockType block = ChunkMeshing::sampleBlock(input, x, y, z);
    return block != BlockType::Air &&
           !isPlant(block) &&
           !isLiquid(block);
}

glm::vec2 flowVector(
    const ChunkMeshInput& input,
    int x,
    int y,
    int z,
    BlockType liquid)
{
    constexpr std::array<glm::ivec2, 4> directions{{
        {-1, 0}, {0, -1}, {1, 0}, {0, 1}
    }};
    glm::vec2 flow(0.0f);
    const int currentLevel = effectiveLevel(input, x, y, z, liquid);

    for (const glm::ivec2& direction : directions)
    {
        const int neighbourX = x + direction.x;
        const int neighbourZ = z + direction.y;
        int neighbourLevel = effectiveLevel(
            input, neighbourX, y, neighbourZ, liquid
        );
        if (neighbourLevel < 0)
        {
            if (!blocksFlow(input, neighbourX, y, neighbourZ))
            {
                neighbourLevel = effectiveLevel(
                    input, neighbourX, y - 1, neighbourZ, liquid
                );
                if (neighbourLevel >= 0)
                {
                    const int difference =
                        neighbourLevel - (currentLevel - 8);
                    flow += glm::vec2(direction) *
                            static_cast<float>(difference);
                }
            }
        }
        else
        {
            flow += glm::vec2(direction) *
                    static_cast<float>(neighbourLevel - currentLevel);
        }
    }

    const float lengthSquared = glm::dot(flow, flow);
    return lengthSquared > 0.000001f
        ? flow / std::sqrt(lengthSquared)
        : glm::vec2(0.0f);
}

void appendQuad(
    std::vector<ChunkVertex>& vertices,
    std::vector<std::uint32_t>& indices,
    const std::array<glm::vec3, 4>& positions,
    const std::array<glm::vec2, 4>& textureCoordinates,
    float brightness,
    MaterialTexture material)
{
    const std::uint32_t base = static_cast<std::uint32_t>(
        vertices.size()
    );
    for (std::size_t index = 0; index < positions.size(); ++index)
    {
        const glm::vec3& position = positions[index];
        const glm::vec2& uv = textureCoordinates[index];
        vertices.push_back({
            position.x, position.y, position.z,
            uv.x, uv.y,
            brightness, brightness, brightness,
            0.0f, 0.0f,
            1.0f, 1.0f, 1.0f,
            0.0f, 0.0f, materialTextureAttribute(material)
        });
    }
    indices.insert(
        indices.end(),
        {base, base + 1, base + 2, base, base + 2, base + 3}
    );
}
}

int appendFluidMesh(
    ChunkMeshData& output,
    const ChunkMeshInput& input,
    int x,
    int y,
    int z,
    BlockType liquid)
{
    if (!isLiquid(liquid))
        return 0;

    std::vector<ChunkVertex>& vertices = liquid == BlockType::Lava
        ? output.lavaVertices
        : output.waterVertices;
    std::vector<std::uint32_t>& indices = liquid == BlockType::Lava
        ? output.lavaIndices
        : output.waterIndices;
    const FluidMaterials materials = materialsFor(liquid);
    const float worldX = static_cast<float>(
        input.snapshot->getWorldOriginX() + x
    );
    const float worldY = static_cast<float>(y);
    const float worldZ = static_cast<float>(
        input.snapshot->getWorldOriginZ() + z
    );
    const float height00 = cornerHeight(input, x, y, z, liquid);
    const float height01 = cornerHeight(input, x, y, z + 1, liquid);
    const float height11 = cornerHeight(input, x + 1, y, z + 1, liquid);
    const float height10 = cornerHeight(input, x + 1, y, z, liquid);
    int faceCount = 0;

    if (ChunkMeshing::shouldRenderFluidFace(
            liquid,
            ChunkMeshing::sampleBlock(input, x, y + 1, z)))
    {
        const glm::vec2 flow = flowVector(input, x, y, z, liquid);
        std::array<glm::vec2, 4> uv{{
            {0.0f, 0.0f}, {1.0f, 0.0f},
            {1.0f, 1.0f}, {0.0f, 1.0f}
        }};
        MaterialTexture topMaterial = materials.still;
        if (glm::dot(flow, flow) > 0.000001f)
        {
            topMaterial = materials.flow;
            const float angle = std::atan2(flow.y, flow.x) -
                std::numbers::pi_v<float> * 0.5f;
            const float sine = std::sin(angle) * 0.25f;
            const float cosine = std::cos(angle) * 0.25f;

            // RenderBlocks supplies these corners in top-origin atlas UVs.
            // AnimatedTexture uploads upright, bottom-origin OpenGL frames,
            // so V must be inverted after applying the Beta flow rotation.
            // The still-texture coordinates above already make that same
            // conversion through their vertex ordering.
            uv = {
                glm::vec2(0.5f - cosine + sine, 0.5f - cosine - sine),
                glm::vec2(0.5f + cosine + sine, 0.5f - cosine + sine),
                glm::vec2(0.5f + cosine - sine, 0.5f + cosine + sine),
                glm::vec2(0.5f - cosine - sine, 0.5f + cosine - sine)
            };
        }

        appendQuad(
            vertices,
            indices,
            std::array<glm::vec3, 4>{{
                {worldX, worldY + height01, worldZ + 1.0f},
                {worldX + 1.0f, worldY + height11, worldZ + 1.0f},
                {worldX + 1.0f, worldY + height10, worldZ},
                {worldX, worldY + height00, worldZ}
            }},
            uv,
            ChunkMeshing::classicBrightness(std::max(
                ChunkMeshing::sampleLight(input, x, y, z),
                ChunkMeshing::sampleLight(input, x, y + 1, z)
            )),
            topMaterial
        );
        ++faceCount;
    }

    if (ChunkMeshing::shouldRenderFluidFace(
            liquid,
            ChunkMeshing::sampleBlock(input, x, y - 1, z)))
    {
        appendQuad(
            vertices,
            indices,
            std::array<glm::vec3, 4>{{
                {worldX, worldY, worldZ},
                {worldX + 1.0f, worldY, worldZ},
                {worldX + 1.0f, worldY, worldZ + 1.0f},
                {worldX, worldY, worldZ + 1.0f}
            }},
            std::array<glm::vec2, 4>{{
                {0.0f, 0.0f}, {1.0f, 0.0f},
                {1.0f, 1.0f}, {0.0f, 1.0f}
            }},
            ChunkMeshing::classicBrightness(
                ChunkMeshing::sampleLight(input, x, y - 1, z)
            ) * 0.5f,
            materials.still
        );
        ++faceCount;
    }

    const auto appendSide = [&vertices, &indices, &materials](
        const std::array<glm::vec3, 4>& positions,
        float firstHeight,
        float secondHeight,
        float brightness)
    {
        appendQuad(
            vertices,
            indices,
            positions,
            std::array<glm::vec2, 4>{{
                {0.0f, 0.0f}, {1.0f, 0.0f},
                {1.0f, secondHeight}, {0.0f, firstHeight}
            }},
            brightness,
            materials.flow
        );
    };

    if (ChunkMeshing::shouldRenderFluidFace(
            liquid, ChunkMeshing::sampleBlock(input, x, y, z - 1)))
    {
        appendSide(
            std::array<glm::vec3, 4>{{
                {worldX + 1.0f, worldY, worldZ},
                {worldX, worldY, worldZ},
                {worldX, worldY + height00, worldZ},
                {worldX + 1.0f, worldY + height10, worldZ}
            }},
            height10, height00,
            ChunkMeshing::classicBrightness(
                ChunkMeshing::sampleLight(input, x, y, z - 1)
            ) * 0.8f
        );
        ++faceCount;
    }
    if (ChunkMeshing::shouldRenderFluidFace(
            liquid, ChunkMeshing::sampleBlock(input, x, y, z + 1)))
    {
        appendSide(
            std::array<glm::vec3, 4>{{
                {worldX, worldY, worldZ + 1.0f},
                {worldX + 1.0f, worldY, worldZ + 1.0f},
                {worldX + 1.0f, worldY + height11, worldZ + 1.0f},
                {worldX, worldY + height01, worldZ + 1.0f}
            }},
            height01, height11,
            ChunkMeshing::classicBrightness(
                ChunkMeshing::sampleLight(input, x, y, z + 1)
            ) * 0.8f
        );
        ++faceCount;
    }
    if (ChunkMeshing::shouldRenderFluidFace(
            liquid, ChunkMeshing::sampleBlock(input, x - 1, y, z)))
    {
        appendSide(
            std::array<glm::vec3, 4>{{
                {worldX, worldY, worldZ},
                {worldX, worldY, worldZ + 1.0f},
                {worldX, worldY + height01, worldZ + 1.0f},
                {worldX, worldY + height00, worldZ}
            }},
            height00, height01,
            ChunkMeshing::classicBrightness(
                ChunkMeshing::sampleLight(input, x - 1, y, z)
            ) * 0.6f
        );
        ++faceCount;
    }
    if (ChunkMeshing::shouldRenderFluidFace(
            liquid, ChunkMeshing::sampleBlock(input, x + 1, y, z)))
    {
        appendSide(
            std::array<glm::vec3, 4>{{
                {worldX + 1.0f, worldY, worldZ + 1.0f},
                {worldX + 1.0f, worldY, worldZ},
                {worldX + 1.0f, worldY + height10, worldZ},
                {worldX + 1.0f, worldY + height11, worldZ + 1.0f}
            }},
            height11, height10,
            ChunkMeshing::classicBrightness(
                ChunkMeshing::sampleLight(input, x + 1, y, z)
            ) * 0.6f
        );
        ++faceCount;
    }

    return faceCount;
}
