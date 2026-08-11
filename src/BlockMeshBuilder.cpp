#include "BlockMeshBuilder.h"

#include "BlockShape.h"
#include "BlockModelSeed.h"
#include "ChunkMeshing.h"
#include "FluidMeshBuilder.h"
#include "TextureAtlas.h"
#include "client/render/ModelBakery.h"
#include "content/ContentCatalog.h"

#include <array>
#include <cmath>
#include <optional>

namespace
{
struct BoxFace
{
    std::array<glm::vec3, 4> vertices;
    glm::ivec3 normal;
    BlockFace face;
    bool touchesVoxelBoundary;
};

std::array<BoxFace, 6> makeFaces(const BlockBox& box)
{
    const glm::vec3& minimum = box.minimum;
    const glm::vec3& maximum = box.maximum;
    constexpr float epsilon = 0.00001f;
    return {{
        BoxFace{std::array<glm::vec3, 4>{{
            glm::vec3(maximum.x, minimum.y, minimum.z),
            glm::vec3(minimum.x, minimum.y, minimum.z),
            glm::vec3(minimum.x, maximum.y, minimum.z),
            glm::vec3(maximum.x, maximum.y, minimum.z)
        }}, glm::ivec3(0, 0, -1), BlockFace::Back, minimum.z <= epsilon},
        BoxFace{std::array<glm::vec3, 4>{{
            glm::vec3(minimum.x, minimum.y, maximum.z),
            glm::vec3(maximum.x, minimum.y, maximum.z),
            glm::vec3(maximum.x, maximum.y, maximum.z),
            glm::vec3(minimum.x, maximum.y, maximum.z)
        }}, glm::ivec3(0, 0, 1), BlockFace::Front,
        maximum.z >= 1.0f - epsilon},
        BoxFace{std::array<glm::vec3, 4>{{
            glm::vec3(minimum.x, minimum.y, minimum.z),
            glm::vec3(minimum.x, minimum.y, maximum.z),
            glm::vec3(minimum.x, maximum.y, maximum.z),
            glm::vec3(minimum.x, maximum.y, minimum.z)
        }}, glm::ivec3(-1, 0, 0), BlockFace::Left, minimum.x <= epsilon},
        BoxFace{std::array<glm::vec3, 4>{{
            glm::vec3(maximum.x, minimum.y, maximum.z),
            glm::vec3(maximum.x, minimum.y, minimum.z),
            glm::vec3(maximum.x, maximum.y, minimum.z),
            glm::vec3(maximum.x, maximum.y, maximum.z)
        }}, glm::ivec3(1, 0, 0), BlockFace::Right,
        maximum.x >= 1.0f - epsilon},
        BoxFace{std::array<glm::vec3, 4>{{
            glm::vec3(minimum.x, minimum.y, minimum.z),
            glm::vec3(maximum.x, minimum.y, minimum.z),
            glm::vec3(maximum.x, minimum.y, maximum.z),
            glm::vec3(minimum.x, minimum.y, maximum.z)
        }}, glm::ivec3(0, -1, 0), BlockFace::Bottom, minimum.y <= epsilon},
        BoxFace{std::array<glm::vec3, 4>{{
            glm::vec3(minimum.x, maximum.y, maximum.z),
            glm::vec3(maximum.x, maximum.y, maximum.z),
            glm::vec3(maximum.x, maximum.y, minimum.z),
            glm::vec3(minimum.x, maximum.y, minimum.z)
        }}, glm::ivec3(0, 1, 0), BlockFace::Top,
        maximum.y >= 1.0f - epsilon}
    }};
}

AtlasUV cropUv(
    const AtlasUV& tile,
    BlockFace face,
    const BlockBox& box)
{
    float minimumU = 0.0f;
    float maximumU = 1.0f;
    float minimumV = 0.0f;
    float maximumV = 1.0f;
    switch (face)
    {
        case BlockFace::Back:
            minimumU = 1.0f - box.maximum.x;
            maximumU = 1.0f - box.minimum.x;
            minimumV = box.minimum.y;
            maximumV = box.maximum.y;
            break;
        case BlockFace::Front:
            minimumU = box.minimum.x;
            maximumU = box.maximum.x;
            minimumV = box.minimum.y;
            maximumV = box.maximum.y;
            break;
        case BlockFace::Left:
            minimumU = box.minimum.z;
            maximumU = box.maximum.z;
            minimumV = box.minimum.y;
            maximumV = box.maximum.y;
            break;
        case BlockFace::Right:
            minimumU = 1.0f - box.maximum.z;
            maximumU = 1.0f - box.minimum.z;
            minimumV = box.minimum.y;
            maximumV = box.maximum.y;
            break;
        case BlockFace::Bottom:
            minimumU = box.minimum.x;
            maximumU = box.maximum.x;
            minimumV = box.minimum.z;
            maximumV = box.maximum.z;
            break;
        case BlockFace::Top:
            minimumU = box.minimum.x;
            maximumU = box.maximum.x;
            minimumV = 1.0f - box.maximum.z;
            maximumV = 1.0f - box.minimum.z;
            break;
    }

    const float tileWidth = tile.maxU - tile.minU;
    const float tileHeight = tile.maxV - tile.minV;
    return {
        tile.minU + tileWidth * minimumU,
        tile.minV + tileHeight * minimumV,
        tile.minU + tileWidth * maximumU,
        tile.minV + tileHeight * maximumV
    };
}

bool blocksAmbientLight(mc::content::BlockState state)
{
    return getBlockShape(state).occludesNeighbourFaces;
}

float directionalShade(const glm::ivec3& normal)
{
    if (normal.y > 0) return 1.0f;
    if (normal.y < 0) return 0.5f;
    if (normal.z != 0) return 0.8f;
    return 0.6f;
}

void vertexTangents(
    const glm::ivec3& normal,
    const glm::vec3& vertex,
    glm::ivec3& tangentA,
    glm::ivec3& tangentB)
{
    const glm::vec3 centred = vertex - glm::vec3(0.5f);
    if (normal.x != 0)
    {
        tangentA = {0, centred.y < 0.0f ? -1 : 1, 0};
        tangentB = {0, 0, centred.z < 0.0f ? -1 : 1};
    }
    else if (normal.y != 0)
    {
        tangentA = {centred.x < 0.0f ? -1 : 1, 0, 0};
        tangentB = {0, 0, centred.z < 0.0f ? -1 : 1};
    }
    else
    {
        tangentA = {centred.x < 0.0f ? -1 : 1, 0, 0};
        tangentB = {0, centred.y < 0.0f ? -1 : 1, 0};
    }
}

float vertexBrightness(
    const ChunkMeshInput& input,
    int x,
    int y,
    int z,
    const glm::ivec3& normal,
    const glm::vec3& vertex)
{
    const glm::ivec3 faceSample{x + normal.x, y + normal.y, z + normal.z};
    const int centreLight = ChunkMeshing::sampleLight(
        input, faceSample.x, faceSample.y, faceSample.z
    );
    if (!input.smoothLighting)
    {
        return ChunkMeshing::classicBrightness(centreLight) *
               directionalShade(normal);
    }

    glm::ivec3 sideAOffset;
    glm::ivec3 sideBOffset;
    vertexTangents(normal, vertex, sideAOffset, sideBOffset);

    const glm::ivec3 sideA = faceSample + sideAOffset;
    const glm::ivec3 sideB = faceSample + sideBOffset;
    const glm::ivec3 corner = faceSample + sideAOffset + sideBOffset;
    const bool sideABlocked = blocksAmbientLight(
        ChunkMeshing::sampleState(input, sideA.x, sideA.y, sideA.z)
    );
    const bool sideBBlocked = blocksAmbientLight(
        ChunkMeshing::sampleState(input, sideB.x, sideB.y, sideB.z)
    );

    const int sideALight = ChunkMeshing::sampleLight(
        input, sideA.x, sideA.y, sideA.z
    );
    const int sideBLight = ChunkMeshing::sampleLight(
        input, sideB.x, sideB.y, sideB.z
    );
    const int cornerLight = sideABlocked && sideBBlocked
        ? sideALight
        : ChunkMeshing::sampleLight(input, corner.x, corner.y, corner.z);
    const float smoothLight =
        (ChunkMeshing::classicBrightness(centreLight) +
         ChunkMeshing::classicBrightness(sideALight) +
         ChunkMeshing::classicBrightness(sideBLight) +
         ChunkMeshing::classicBrightness(cornerLight)) * 0.25f;

    const bool cornerBlocked = sideABlocked && sideBBlocked
        ? true
        : blocksAmbientLight(
            ChunkMeshing::sampleState(input, corner.x, corner.y, corner.z)
          );
    const int occluders = static_cast<int>(sideABlocked) +
        static_cast<int>(sideBBlocked) + static_cast<int>(cornerBlocked);
    constexpr float ambientOcclusion[4] = {1.00f, 0.88f, 0.76f, 0.66f};
    return smoothLight * ambientOcclusion[occluders] *
           directionalShade(normal);
}

glm::vec3 tintFor(
    BlockType block,
    BlockFace face,
    float temperature,
    float humidity,
    BiomeId biome,
    int worldX,
    int worldZ,
    const BiomeColorMap& map)
{
    if (block == BlockType::Grass && face == BlockFace::Top)
        return map.getGrassColor(
            temperature, humidity, biome, worldX, worldZ
        );
    if (block == BlockType::TallGrass || block == BlockType::Fern)
        return map.getGrassColor(
            temperature, humidity, biome, worldX, worldZ
        );
    if (block == BlockType::Vine ||
        (isLeaf(block) && block != BlockType::SpruceLeaves &&
         block != BlockType::BirchLeaves))
        return map.getFoliageColor(
            temperature, humidity, biome, worldX, worldZ
        );
    if (block == BlockType::SpruceLeaves)
        return {0x61 / 255.0f, 0x99 / 255.0f, 0x61 / 255.0f};
    if (block == BlockType::BirchLeaves)
        return {0x80 / 255.0f, 0xA7 / 255.0f, 0x55 / 255.0f};
    return {1.0f, 1.0f, 1.0f};
}

glm::vec3 blendedTintFor(
    BlockType block,
    BlockFace face,
    const ChunkMeshInput& input,
    int x,
    int z,
    const BiomeColorMap& map)
{
    const bool biomeTinted =
        (block == BlockType::Grass && face == BlockFace::Top) ||
        block == BlockType::TallGrass || block == BlockType::Fern ||
        block == BlockType::Vine ||
        (isLeaf(block) && block != BlockType::SpruceLeaves &&
         block != BlockType::BirchLeaves);
    if (!biomeTinted)
    {
        return tintFor(
            block,
            face,
            input.snapshot->getTemperature(x, z),
            input.snapshot->getHumidity(x, z),
            input.snapshot->getBiome(x, z),
            input.snapshot->getWorldOriginX() + x,
            input.snapshot->getWorldOriginZ() + z,
            map
        );
    }

    glm::vec3 colour(0.0f);
    for (int offsetX = -1; offsetX <= 1; ++offsetX)
    {
        for (int offsetZ = -1; offsetZ <= 1; ++offsetZ)
        {
            const int sampleX = x + offsetX;
            const int sampleZ = z + offsetZ;
            colour += tintFor(
                block,
                face,
                input.snapshot->getTemperature(sampleX, sampleZ),
                input.snapshot->getHumidity(sampleX, sampleZ),
                input.snapshot->getBiome(sampleX, sampleZ),
                input.snapshot->getWorldOriginX() + sampleX,
                input.snapshot->getWorldOriginZ() + sampleZ,
                map
            );
        }
    }
    return colour / 9.0f;
}

void appendFace(
    std::vector<ChunkVertex>& vertices,
    std::vector<std::uint32_t>& indices,
    const BoxFace& face,
    const AtlasUV& uv,
    const glm::vec3& worldMinimum,
    const glm::vec3& tint,
    const AtlasUV* overlay,
    const glm::vec3& overlayTint,
    bool leaf,
    const ChunkMeshInput& input,
    int x,
    int y,
    int z)
{
    const std::uint32_t base = static_cast<std::uint32_t>(vertices.size());
    const std::array<glm::vec2, 4> textureCoordinates{{
        {uv.minU, uv.minV}, {uv.maxU, uv.minV},
        {uv.maxU, uv.maxV}, {uv.minU, uv.maxV}
    }};
    std::array<glm::vec2, 4> overlayCoordinates{};
    if (overlay != nullptr)
    {
        overlayCoordinates = {
            glm::vec2(overlay->minU, overlay->minV),
            glm::vec2(overlay->maxU, overlay->minV),
            glm::vec2(overlay->maxU, overlay->maxV),
            glm::vec2(overlay->minU, overlay->maxV)
        };
    }

    std::array<float, 4> brightness{};
    for (std::size_t index = 0; index < face.vertices.size(); ++index)
    {
        brightness[index] = vertexBrightness(
            input, x, y, z, face.normal, face.vertices[index]
        );
        const glm::vec3 position = face.vertices[index] + worldMinimum;
        vertices.push_back({
            position.x, position.y, position.z,
            textureCoordinates[index].x, textureCoordinates[index].y,
            tint.r * brightness[index],
            tint.g * brightness[index],
            tint.b * brightness[index],
            overlayCoordinates[index].x, overlayCoordinates[index].y,
            overlayTint.r * brightness[index],
            overlayTint.g * brightness[index],
            overlayTint.b * brightness[index],
            overlay != nullptr ? 1.0f : 0.0f,
            leaf ? 1.0f : 0.0f,
            materialTextureAttribute(MaterialTexture::Atlas)
        });
    }

    if (brightness[0] + brightness[2] > brightness[1] + brightness[3])
        indices.insert(indices.end(),
            {base, base + 1, base + 3, base + 1, base + 2, base + 3});
    else
        indices.insert(indices.end(),
            {base, base + 1, base + 2, base, base + 2, base + 3});
}

int appendBoxes(
    ChunkMeshData& output,
    const ChunkMeshInput& input,
    const BiomeColorMap& colourMap,
    int x,
    int y,
    int z,
    mc::content::BlockState state,
    const BlockShapeDefinition& shape)
{
    const BlockType block = state.block();
    std::vector<ChunkVertex>* vertices = &output.opaqueVertices;
    std::vector<std::uint32_t>* indices = &output.opaqueIndices;
    if (isTranslucent(block) && !isLiquid(block) && !isLeaf(block))
    {
        // The existing back-to-front water pass is the generic translucent
        // block layer. Atlas-backed glass and ice retain depth testing while
        // avoiding the opaque/cutout x-ray artifacts of the old path.
        vertices = &output.waterVertices;
        indices = &output.waterIndices;
    }
    else if (isCutout(block) && !isLeaf(block))
    {
        vertices = &output.cutoutVertices;
        indices = &output.cutoutIndices;
    }

    const glm::vec3 worldMinimum(
        static_cast<float>(input.snapshot->getWorldOriginX() + x),
        static_cast<float>(y),
        static_cast<float>(input.snapshot->getWorldOriginZ() + z)
    );
    int faceCount = 0;
    for (const BlockBox& box : shape.renderBoxes)
    {
        for (const BoxFace& face : makeFaces(box))
        {
            bool fastLeafVisible = isLeaf(block);
            bool fancyLeafVisible = isLeaf(block);
            if (face.touchesVoxelBoundary)
            {
                const BlockType neighbour = ChunkMeshing::sampleBlock(
                    input,
                    x + face.normal.x,
                    y + face.normal.y,
                    z + face.normal.z
                );
                if (isLeaf(block))
                {
                    fastLeafVisible =
                        ChunkMeshing::shouldRenderFastLeafFace(
                            block, neighbour
                        );
                    fancyLeafVisible =
                        ChunkMeshing::shouldRenderFancyLeafFace(neighbour);
                    if (!fastLeafVisible && !fancyLeafVisible)
                        continue;
                }
                else if (!ChunkMeshing::shouldRenderFace(block, neighbour))
                {
                    continue;
                }
            }

            const AtlasUV tileUv = TextureAtlas::getBlockUV(state, face.face);
            const AtlasUV uv = cropUv(tileUv, face.face, box);
            const bool grassSide = block == BlockType::Grass &&
                face.face != BlockFace::Top &&
                face.face != BlockFace::Bottom;
            AtlasUV overlayUv{};
            const AtlasUV* overlay = nullptr;
            glm::vec3 overlayTint(1.0f);
            if (grassSide)
            {
                if (const std::optional<AtlasUV> registryOverlay =
                        TextureAtlas::getBlockOverlayUV(state, face.face))
                {
                    overlayUv = cropUv(*registryOverlay, face.face, box);
                    overlay = &overlayUv;
                    overlayTint = blendedTintFor(
                        BlockType::Grass,
                        BlockFace::Top,
                        input,
                        x,
                        z,
                        colourMap
                    );
                }
            }

            const glm::vec3 tint = blendedTintFor(
                block, face.face, input, x, z, colourMap
            );
            const auto appendTo = [&](
                std::vector<ChunkVertex>& targetVertices,
                std::vector<std::uint32_t>& targetIndices)
            {
                appendFace(
                    targetVertices,
                    targetIndices,
                    face,
                    uv,
                    worldMinimum,
                    tint,
                    overlay,
                    overlayTint,
                    isLeaf(block),
                    input,
                    x,
                    y,
                    z
                );
            };

            if (isLeaf(block))
            {
                if (input.fastLeaves && fastLeafVisible)
                {
                    appendTo(
                        output.fastLeafVertices,
                        output.fastLeafIndices
                    );
                }
                if (!input.fastLeaves && fancyLeafVisible)
                {
                    appendTo(
                        output.fancyLeafVertices,
                        output.fancyLeafIndices
                    );
                    ++faceCount;
                }
            }
            else
            {
                appendTo(*vertices, *indices);
                ++faceCount;
            }
        }
    }
    return faceCount;
}

int appendCrossedPlanes(
    ChunkMeshData& output,
    const ChunkMeshInput& input,
    const BiomeColorMap& colourMap,
    int x,
    int y,
    int z,
    mc::content::BlockState state)
{
    const BlockType block = state.block();
    constexpr float radius = 0.45f;
    const std::array<std::array<glm::vec3, 4>, 2> planes{{
        {{{-radius, -0.5f, -radius}, {radius, -0.5f, radius},
          {radius, 0.5f, radius}, {-radius, 0.5f, -radius}}},
        {{{-radius, -0.5f, radius}, {radius, -0.5f, -radius},
          {radius, 0.5f, -radius}, {-radius, 0.5f, radius}}}
    }};
    const AtlasUV uv = TextureAtlas::getBlockUV(state, BlockFace::Front);
    const std::array<glm::vec2, 4> textureCoordinates{{
        {uv.minU, uv.minV}, {uv.maxU, uv.minV},
        {uv.maxU, uv.maxV}, {uv.minU, uv.maxV}
    }};
    const float brightness = ChunkMeshing::classicBrightness(
        ChunkMeshing::sampleLight(input, x, y, z)
    );
    const glm::vec3 tint = blendedTintFor(
        block, BlockFace::Front, input, x, z, colourMap
    );
    const glm::vec3 origin(
        static_cast<float>(input.snapshot->getWorldOriginX() + x) + 0.5f,
        static_cast<float>(y) + 0.5f,
        static_cast<float>(input.snapshot->getWorldOriginZ() + z) + 0.5f
    );
    for (const auto& plane : planes)
    {
        const std::uint32_t base = static_cast<std::uint32_t>(
            output.cutoutVertices.size()
        );
        for (std::size_t index = 0; index < plane.size(); ++index)
        {
            const glm::vec3 position = plane[index] + origin;
            output.cutoutVertices.push_back({
                position.x, position.y, position.z,
                textureCoordinates[index].x, textureCoordinates[index].y,
                tint.r * brightness, tint.g * brightness, tint.b * brightness,
                0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
                0.0f, 0.0f,
                materialTextureAttribute(MaterialTexture::Atlas)
            });
        }
        output.cutoutIndices.insert(output.cutoutIndices.end(),
            {base, base + 1, base + 2, base, base + 2, base + 3,
             base + 2, base + 1, base, base + 3, base + 2, base});
    }
    return 4;
}

int appendWallPlane(
    ChunkMeshData& output,
    const ChunkMeshInput& input,
    int x,
    int y,
    int z,
    mc::content::BlockState state)
{
    const AtlasUV uv = TextureAtlas::getBlockUV(state, BlockFace::Front);
    const float brightness = ChunkMeshing::classicBrightness(
        ChunkMeshing::sampleLight(input, x, y, z)
    ) * 0.8f;
    const float worldX = static_cast<float>(input.snapshot->getWorldOriginX() + x);
    const float worldZ = static_cast<float>(input.snapshot->getWorldOriginZ() + z);
    constexpr float planeOffset = 0.01f;
    const std::array<glm::vec3, 4> positions{{
        {worldX, static_cast<float>(y), worldZ + planeOffset},
        {worldX + 1.0f, static_cast<float>(y), worldZ + planeOffset},
        {worldX + 1.0f, static_cast<float>(y + 1), worldZ + planeOffset},
        {worldX, static_cast<float>(y + 1), worldZ + planeOffset}
    }};
    const std::array<glm::vec2, 4> textureCoordinates{{
        {uv.minU, uv.minV}, {uv.maxU, uv.minV},
        {uv.maxU, uv.maxV}, {uv.minU, uv.maxV}
    }};
    const std::uint32_t base = static_cast<std::uint32_t>(
        output.cutoutVertices.size()
    );
    for (std::size_t index = 0; index < positions.size(); ++index)
    {
        output.cutoutVertices.push_back({
            positions[index].x, positions[index].y, positions[index].z,
            textureCoordinates[index].x, textureCoordinates[index].y,
            brightness, brightness, brightness,
            0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
            0.0f, 0.0f,
            materialTextureAttribute(MaterialTexture::Atlas)
        });
    }
    output.cutoutIndices.insert(output.cutoutIndices.end(),
        {base, base + 1, base + 2, base, base + 2, base + 3,
         base + 2, base + 1, base, base + 3, base + 2, base});
    return 1;
}

glm::ivec3 faceNormal(BlockFace face)
{
    switch (face)
    {
        case BlockFace::Back: return {0, 0, -1};
        case BlockFace::Front: return {0, 0, 1};
        case BlockFace::Left: return {-1, 0, 0};
        case BlockFace::Right: return {1, 0, 0};
        case BlockFace::Bottom: return {0, -1, 0};
        case BlockFace::Top: return {0, 1, 0};
    }
    return {0, 1, 0};
}

glm::vec3 registryTint(
    const mc::content::BlockDefinition& definition,
    const ChunkMeshInput& input,
    const BiomeColorMap& colourMap,
    int x,
    int z,
    int tintIndex)
{
    if (tintIndex < 0 || definition.behaviour.tint == mc::content::BlockTint::None)
        return {1.0f, 1.0f, 1.0f};
    if (definition.behaviour.tint == mc::content::BlockTint::SpruceFoliage)
        return {0x61 / 255.0f, 0x99 / 255.0f, 0x61 / 255.0f};
    if (definition.behaviour.tint == mc::content::BlockTint::BirchFoliage)
        return {0x80 / 255.0f, 0xA7 / 255.0f, 0x55 / 255.0f};

    glm::vec3 colour(0.0f);
    for (int offsetX = -1; offsetX <= 1; ++offsetX)
    {
        for (int offsetZ = -1; offsetZ <= 1; ++offsetZ)
        {
            const int sampleX = x + offsetX;
            const int sampleZ = z + offsetZ;
            const float temperature = input.snapshot->getTemperature(sampleX, sampleZ);
            const float humidity = input.snapshot->getHumidity(sampleX, sampleZ);
            const BiomeId biome = input.snapshot->getBiome(sampleX, sampleZ);
            const int worldX = input.snapshot->getWorldOriginX() + sampleX;
            const int worldZ = input.snapshot->getWorldOriginZ() + sampleZ;
            colour += definition.behaviour.tint == mc::content::BlockTint::Grass
                ? colourMap.getGrassColor(temperature, humidity, biome, worldX, worldZ)
                : colourMap.getFoliageColor(temperature, humidity, biome, worldX, worldZ);
        }
    }
    return colour / 9.0f;
}

int appendBakedModel(
    ChunkMeshData& output,
    const ChunkMeshInput& input,
    const BiomeColorMap& colourMap,
    int x,
    int y,
    int z,
    mc::content::BlockState state,
    const mc::content::BlockDefinition& definition,
    const mc::client::BakedModel& model)
{
    std::vector<ChunkVertex>* vertices = &output.opaqueVertices;
    std::vector<std::uint32_t>* indices = &output.opaqueIndices;
    const bool leaf = definition.behaviour.traits.leaf;
    if (leaf)
    {
        if (input.fastLeaves)
        {
            vertices = &output.fastLeafVertices;
            indices = &output.fastLeafIndices;
        }
        else
        {
            vertices = &output.fancyLeafVertices;
            indices = &output.fancyLeafIndices;
        }
    }
    else if (definition.renderLayer == mc::content::RenderLayer::Cutout ||
             definition.renderLayer == mc::content::RenderLayer::CutoutMipped)
    {
        vertices = &output.cutoutVertices;
        indices = &output.cutoutIndices;
    }
    else if (definition.renderLayer == mc::content::RenderLayer::Translucent)
    {
        vertices = &output.waterVertices;
        indices = &output.waterIndices;
    }

    const glm::vec3 worldOrigin(
        static_cast<float>(input.snapshot->getWorldOriginX() + x),
        static_cast<float>(y),
        static_cast<float>(input.snapshot->getWorldOriginZ() + z)
    );
    int faceCount = 0;
    for (const mc::client::BakedQuad& quad : model.quads)
    {
        const glm::ivec3 normal = faceNormal(quad.face);
        if (quad.cullFace)
        {
            const glm::ivec3 cullNormal = faceNormal(*quad.cullFace);
            const mc::content::BlockState neighbour = ChunkMeshing::sampleState(
                input, x + cullNormal.x, y + cullNormal.y, z + cullNormal.z
            );
            if (leaf)
            {
                const bool neighbourLeaf = [&]()
                {
                    const mc::content::ContentCatalog* catalog =
                        mc::content::ContentCatalog::active();
                    const mc::content::BlockDefinition* neighbourDefinition =
                        catalog == nullptr ? nullptr : catalog->block(neighbour);
                    return neighbourDefinition != nullptr &&
                           neighbourDefinition->behaviour.traits.leaf;
                }();
                if ((input.fastLeaves && neighbourLeaf) ||
                    (!neighbourLeaf && ChunkMeshing::occludesNeighbourFace(neighbour)))
                    continue;
            }
            else if (!ChunkMeshing::shouldRenderFace(state, neighbour))
            {
                continue;
            }
        }

        const AtlasUV* texture = TextureAtlas::getTextureUV(quad.texture);
        if (texture == nullptr)
            continue;
        const glm::vec3 tint = registryTint(
            definition, input, colourMap, x, z, quad.tintIndex
        );
        const std::uint32_t base = static_cast<std::uint32_t>(vertices->size());
        std::array<float, 4> brightness{};
        for (std::size_t vertex = 0; vertex < quad.positions.size(); ++vertex)
        {
            brightness[vertex] = quad.shade
                ? vertexBrightness(input, x, y, z, normal, quad.positions[vertex])
                : ChunkMeshing::classicBrightness(
                    ChunkMeshing::sampleLight(input, x + normal.x, y + normal.y, z + normal.z)
                  );
            const glm::vec3 position = quad.positions[vertex] + worldOrigin;
            const float u = texture->minU +
                (texture->maxU - texture->minU) *
                (quad.textureCoordinates[vertex].x / 16.0f);
            const float v = texture->minV +
                (texture->maxV - texture->minV) *
                (quad.textureCoordinates[vertex].y / 16.0f);
            vertices->push_back({
                position.x, position.y, position.z,
                u, v,
                tint.r * brightness[vertex],
                tint.g * brightness[vertex],
                tint.b * brightness[vertex],
                0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
                0.0f, leaf ? 1.0f : 0.0f,
                materialTextureAttribute(MaterialTexture::Atlas)
            });
        }
        if (brightness[0] + brightness[2] > brightness[1] + brightness[3])
            indices->insert(indices->end(),
                {base, base + 1, base + 3, base + 1, base + 2, base + 3});
        else
            indices->insert(indices->end(),
                {base, base + 1, base + 2, base, base + 2, base + 3});
        ++faceCount;
    }
    return faceCount;
}
}

int appendBlockMesh(
    ChunkMeshData& output,
    const ChunkMeshInput& input,
    const BiomeColorMap& colourMap,
    int x,
    int y,
    int z,
    mc::content::BlockState state)
{
    state = ChunkMeshing::actualState(input, x, y, z, state);
    if (state.blockRuntimeId() >
        static_cast<mc::core::RuntimeId>(BlockType::Cobweb))
    {
        const mc::content::ContentCatalog* catalog =
            mc::content::ContentCatalog::active();
        const mc::content::BlockDefinition* definition =
            catalog == nullptr ? nullptr : catalog->block(state);
        const int worldX = input.snapshot->getWorldOriginX() + x;
        const int worldZ = input.snapshot->getWorldOriginZ() + z;
        const mc::client::BakedModel* model =
            TextureAtlas::getBakedBlockModel(
                state, blockModelSeed(worldX, y, worldZ)
            );
        if (definition != nullptr && model != nullptr)
            return appendBakedModel(
                output, input, colourMap, x, y, z, state, *definition, *model
            );
        return 0;
    }
    const BlockType block = state.block();
    const BlockShapeDefinition& shape = getBlockShape(block);
    switch (shape.renderShape)
    {
        case BlockRenderShape::None:
            return 0;
        case BlockRenderShape::Boxes:
            return appendBoxes(
                output, input, colourMap, x, y, z, state, shape
            );
        case BlockRenderShape::CrossedPlanes:
            return appendCrossedPlanes(
                output, input, colourMap, x, y, z, state
            );
        case BlockRenderShape::WallPlane:
            return appendWallPlane(output, input, x, y, z, state);
        case BlockRenderShape::Fluid:
            return appendFluidMesh(output, input, x, y, z, block);
    }
    return 0;
}
