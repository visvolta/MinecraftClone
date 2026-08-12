#include "ItemEntityRenderer.h"

#include "AssetPaths.h"
#include "Atmosphere.h"
#include "BlockShape.h"
#include "ItemAtlas.h"
#include "ItemEntity.h"
#include "Shader.h"
#include "Texture2D.h"
#include "TextureAtlas.h"
#include "BiomeColorMap.h"
#include "client/render/ModelBakery.h"
#include "content/ContentCatalog.h"
#include "content/resources/ResourcePack.h"

#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <cmath>
#include <cstddef>
#include <random>

namespace
{
struct ItemFace
{
    std::array<glm::vec3, 4> positions;
    BlockFace blockFace;
    glm::vec3 shade;
};

constexpr std::array<unsigned int, 6> FaceIndices{0, 1, 2, 0, 2, 3};
constexpr std::array<unsigned int, 12> DoubleSidedIndices{
    0, 1, 2, 0, 2, 3,
    2, 1, 0, 3, 2, 0
};

std::uint32_t itemMeshKey(const ItemStack& stack) noexcept
{
    const std::uint32_t item =
        static_cast<std::uint32_t>(static_cast<std::uint16_t>(stack.item));
    const std::uint32_t metadata = isBlockItem(stack.item)
        ? static_cast<std::uint32_t>(stack.damage)
        : 0U;
    return item | (metadata << 16U);
}

glm::vec3 packedColour(std::uint32_t colour) noexcept
{
    return {
        static_cast<float>((colour >> 16U) & 0xFFU) / 255.0f,
        static_cast<float>((colour >> 8U) & 0xFFU) / 255.0f,
        static_cast<float>(colour & 0xFFU) / 255.0f
    };
}

const BiomeColorMap& itemColourMap()
{
    static const BiomeColorMap map(
        AssetPaths::get("textures/grasscolor.png"),
        AssetPaths::get("textures/foliage.png")
    );
    return map;
}

glm::vec3 vanillaItemTint(
    const mc::content::BlockDefinition* definition,
    int tintIndex)
{
    if (definition == nullptr || tintIndex < 0)
        return glm::vec3(1.0f);

    switch (definition->behaviour.tint)
    {
        case mc::content::BlockTint::Grass:
            return itemColourMap().getGrassColor(
                0.5f, 1.0f, VanillaBiomes::Plains, 0, 0
            );
        case mc::content::BlockTint::Foliage:
            return packedColour(4764952U);
        case mc::content::BlockTint::SpruceFoliage:
            return packedColour(6396257U);
        case mc::content::BlockTint::BirchFoliage:
            return packedColour(8431445U);
        case mc::content::BlockTint::None:
            return glm::vec3(1.0f);
    }
    return glm::vec3(1.0f);
}

glm::vec3 itemDirectionalShade(BlockFace face, bool shade) noexcept
{
    if (!shade) return glm::vec3(1.0f);
    switch (face)
    {
        case BlockFace::Top: return glm::vec3(1.0f);
        case BlockFace::Bottom: return glm::vec3(0.5f);
        case BlockFace::Back:
        case BlockFace::Front: return glm::vec3(0.8f);
        case BlockFace::Left:
        case BlockFace::Right: return glm::vec3(0.6f);
    }
    return glm::vec3(1.0f);
}

template <typename VertexType, std::size_t IndexCount>
void appendQuadWithUvs(
    std::vector<VertexType>& vertices,
    const std::array<glm::vec3, 4>& positions,
    const std::array<glm::vec2, 4>& textureCoordinates,
    const glm::vec3& colour,
    const std::array<unsigned int, IndexCount>& indices
);

template <typename VertexType>
bool appendBakedModel(
    std::vector<VertexType>& vertices,
    const mc::client::BakedModel& model,
    const mc::content::BlockDefinition* definition)
{
    const std::size_t before = vertices.size();
    for (const mc::client::BakedQuad& quad : model.quads)
    {
        const AtlasUV* atlasUv = TextureAtlas::getTextureUV(quad.texture);
        if (atlasUv == nullptr)
            continue;

        std::array<glm::vec3, 4> positions = quad.positions;
        for (glm::vec3& position : positions)
            position -= glm::vec3(0.5f);

        std::array<glm::vec2, 4> textureCoordinates{};
        for (std::size_t i = 0; i < textureCoordinates.size(); ++i)
        {
            textureCoordinates[i] = {
                atlasUv->minU +
                    (atlasUv->maxU - atlasUv->minU) *
                    (quad.textureCoordinates[i].x / 16.0f),
                atlasUv->minV +
                    (atlasUv->maxV - atlasUv->minV) *
                    (quad.textureCoordinates[i].y / 16.0f)
            };
        }

        const glm::vec3 colour =
            itemDirectionalShade(quad.face, quad.shade) *
            vanillaItemTint(definition, quad.tintIndex);

        appendQuadWithUvs(
            vertices, positions, textureCoordinates, colour, FaceIndices
        );
    }
    return vertices.size() != before;
}

std::array<ItemFace, 6> cubeFaces(const BlockBox& box)
{
    const glm::vec3 minimum = box.minimum - glm::vec3(0.5f);
    const glm::vec3 maximum = box.maximum - glm::vec3(0.5f);
    return {{
        ItemFace{std::array<glm::vec3, 4>{{
            glm::vec3(maximum.x, minimum.y, minimum.z),
            glm::vec3(minimum.x, minimum.y, minimum.z),
            glm::vec3(minimum.x, maximum.y, minimum.z),
            glm::vec3(maximum.x, maximum.y, minimum.z)
        }}, BlockFace::Back, {0.8f, 0.8f, 0.8f}},
        ItemFace{std::array<glm::vec3, 4>{{
            glm::vec3(minimum.x, minimum.y, maximum.z),
            glm::vec3(maximum.x, minimum.y, maximum.z),
            glm::vec3(maximum.x, maximum.y, maximum.z),
            glm::vec3(minimum.x, maximum.y, maximum.z)
        }}, BlockFace::Front, {0.8f, 0.8f, 0.8f}},
        ItemFace{std::array<glm::vec3, 4>{{
            glm::vec3(minimum.x, minimum.y, minimum.z),
            glm::vec3(minimum.x, minimum.y, maximum.z),
            glm::vec3(minimum.x, maximum.y, maximum.z),
            glm::vec3(minimum.x, maximum.y, minimum.z)
        }}, BlockFace::Left, {0.6f, 0.6f, 0.6f}},
        ItemFace{std::array<glm::vec3, 4>{{
            glm::vec3(maximum.x, minimum.y, maximum.z),
            glm::vec3(maximum.x, minimum.y, minimum.z),
            glm::vec3(maximum.x, maximum.y, minimum.z),
            glm::vec3(maximum.x, maximum.y, maximum.z)
        }}, BlockFace::Right, {0.6f, 0.6f, 0.6f}},
        ItemFace{std::array<glm::vec3, 4>{{
            glm::vec3(minimum.x, minimum.y, minimum.z),
            glm::vec3(maximum.x, minimum.y, minimum.z),
            glm::vec3(maximum.x, minimum.y, maximum.z),
            glm::vec3(minimum.x, minimum.y, maximum.z)
        }}, BlockFace::Bottom, {0.5f, 0.5f, 0.5f}},
        ItemFace{std::array<glm::vec3, 4>{{
            glm::vec3(minimum.x, maximum.y, maximum.z),
            glm::vec3(maximum.x, maximum.y, maximum.z),
            glm::vec3(maximum.x, maximum.y, minimum.z),
            glm::vec3(minimum.x, maximum.y, minimum.z)
        }}, BlockFace::Top, {1.0f, 1.0f, 1.0f}}
    }};
}

template <typename VertexType, std::size_t IndexCount>
void appendQuadWithUvs(
    std::vector<VertexType>& vertices,
    const std::array<glm::vec3, 4>& positions,
    const std::array<glm::vec2, 4>& textureCoordinates,
    const glm::vec3& colour,
    const std::array<unsigned int, IndexCount>& indices)
{
    for (const unsigned int index : indices)
    {
        vertices.push_back({
            positions[index],
            textureCoordinates[index],
            colour
        });
    }
}

template <typename VertexType, std::size_t IndexCount>
void appendQuad(
    std::vector<VertexType>& vertices,
    const std::array<glm::vec3, 4>& positions,
    const AtlasUV& uv,
    const glm::vec3& colour,
    const std::array<unsigned int, IndexCount>& indices)
{
    const std::array<glm::vec2, 4> textureCoordinates{{
        {uv.minU, uv.minV},
        {uv.maxU, uv.minV},
        {uv.maxU, uv.maxV},
        {uv.minU, uv.maxV}
    }};
    appendQuadWithUvs(
        vertices, positions, textureCoordinates, colour, indices
    );
}
}

ItemEntityRenderer::ItemEntityRenderer()
    : shader_(std::make_unique<Shader>(
          AssetPaths::get("shaders/item_entity.vert"),
          AssetPaths::get("shaders/item_entity.frag")))
{
    shader_->use();
    shader_->setInt("atlasTexture", 0);
}

ItemEntityRenderer::~ItemEntityRenderer()
{
    for (const auto& entry : meshes_)
    {
        const GpuMesh& mesh = entry.second;
        if (mesh.vertexBuffer != 0)
            glDeleteBuffers(1, &mesh.vertexBuffer);
        if (mesh.vertexArray != 0)
            glDeleteVertexArrays(1, &mesh.vertexArray);
    }
}

void ItemEntityRenderer::draw(
    const ItemEntity& entity,
    float partialTick,
    const glm::mat4& view,
    const glm::mat4& projection,
    const Texture2D& blockAtlas,
    const ItemAtlas& itemAtlas,
    const AtmosphereState& atmosphere)
{
    if (entity.getStack().empty())
        return;

    const ItemStack& stack = entity.getStack();
    const ItemType item = stack.item;
    const GpuMesh& mesh = meshFor(stack, itemAtlas);
    if (mesh.vertexCount == 0)
        return;

    const float age = static_cast<float>(entity.getAge()) + partialTick;
    const float bob = std::sin(
        age / 10.0f + entity.getHoverStart()
    ) * 0.1f + 0.1f;
    const float rotation = age / 20.0f + entity.getHoverStart();

    shader_->use();
    shader_->setMat4("view", view);
    shader_->setMat4("projection", projection);
    shader_->setFloat(
        "daylightBrightness", atmosphere.daylightBrightness
    );
    shader_->setInt("fogMode", static_cast<int>(atmosphere.fogMode));
    shader_->setVec3("fogColour", atmosphere.fogColour);
    shader_->setFloat("fogStart", atmosphere.fogStart);
    shader_->setFloat("fogEnd", atmosphere.fogEnd);
    shader_->setFloat("fogDensity", atmosphere.fogDensity);
    if (isBlockItem(item))
        blockAtlas.bind(0);
    else
        itemAtlas.texture().bind(0);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glBindVertexArray(mesh.vertexArray);

    int copyCount = 1;
    if (entity.getStack().count > 1) copyCount = 2;
    if (entity.getStack().count > 5) copyCount = 3;
    if (entity.getStack().count > 20) copyCount = 4;

    std::mt19937 offsets(187U);
    std::uniform_real_distribution<float> offset(-0.15f, 0.15f);
    const float scale = isBlockItem(item) ? 0.25f : 0.5f;
    for (int copy = 0; copy < copyCount; ++copy)
    {
        glm::mat4 model(1.0f);
        model = glm::translate(
            model,
            entity.getInterpolatedPosition(partialTick) +
                glm::vec3(0.0f, bob, 0.0f)
        );
        model = glm::rotate(
            model, rotation, glm::vec3(0.0f, 1.0f, 0.0f)
        );
        if (copy > 0)
        {
            model = glm::translate(
                model,
                glm::vec3(offset(offsets), offset(offsets), offset(offsets))
            );
        }
        model = glm::scale(model, glm::vec3(scale));
        shader_->setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, mesh.vertexCount);
    }
    glBindVertexArray(0);
}

const ItemEntityRenderer::GpuMesh& ItemEntityRenderer::meshFor(
    const ItemStack& stack,
    const ItemAtlas& itemAtlas)
{
    auto [iterator, inserted] = meshes_.try_emplace(itemMeshKey(stack));
    if (inserted)
        upload(iterator->second, buildVertices(stack, itemAtlas));
    return iterator->second;
}

std::vector<ItemEntityRenderer::Vertex>
ItemEntityRenderer::buildVertices(
    const ItemStack& stack,
    const ItemAtlas& itemAtlas)
{
    const ItemType item = stack.item;
    std::vector<Vertex> vertices;
    if (!isBlockItem(item))
    {
        const AtlasUV uv = itemAtlas.getItemUV(item);
        constexpr float halfThickness = 1.0f / 32.0f;
        constexpr glm::vec3 colour(1.0f);

        appendQuad(
            vertices,
            {{{-0.5f, -0.5f, halfThickness},
              {0.5f, -0.5f, halfThickness},
              {0.5f, 0.5f, halfThickness},
              {-0.5f, 0.5f, halfThickness}}},
            uv,
            colour,
            FaceIndices
        );
        // The back face's vertices run right-to-left for CCW winding. Flip its
        // U coordinates so the item artwork retains the same handedness when
        // viewed from either side instead of reading as a mirror image.
        appendQuadWithUvs(
            vertices,
            {{{0.5f, -0.5f, -halfThickness},
              {-0.5f, -0.5f, -halfThickness},
              {-0.5f, 0.5f, -halfThickness},
              {0.5f, 0.5f, -halfThickness}}},
            std::array<glm::vec2, 4>{{
                {uv.maxU, uv.minV},
                {uv.minU, uv.minV},
                {uv.minU, uv.maxV},
                {uv.maxU, uv.maxV}
            }},
            colour,
            FaceIndices
        );

        for (int pixelY = 0; pixelY < 16; ++pixelY)
        {
            for (int pixelX = 0; pixelX < 16; ++pixelX)
            {
                if (!itemAtlas.isOpaque(item, pixelX, pixelY))
                    continue;

                const float x0 = -0.5f + pixelX / 16.0f;
                const float x1 = -0.5f + (pixelX + 1) / 16.0f;
                const float y1 = 0.5f - pixelY / 16.0f;
                const float y0 = 0.5f - (pixelY + 1) / 16.0f;
                const float centreU = uv.minU +
                    (pixelX + 0.5f) / 16.0f * (uv.maxU - uv.minU);
                const float centreV = uv.maxV -
                    (pixelY + 0.5f) / 16.0f * (uv.maxV - uv.minV);
                const std::array<glm::vec2, 4> edgeUvs{{
                    {centreU, centreV}, {centreU, centreV},
                    {centreU, centreV}, {centreU, centreV}
                }};

                const auto edge = [&vertices, &edgeUvs](
                    const std::array<glm::vec3, 4>& positions)
                {
                    appendQuadWithUvs(
                        vertices,
                        positions,
                        edgeUvs,
                        glm::vec3(1.0f),
                        FaceIndices
                    );
                };

                if (!itemAtlas.isOpaque(item, pixelX - 1, pixelY))
                {
                    edge({{{x0, y0, -halfThickness},
                           {x0, y0, halfThickness},
                           {x0, y1, halfThickness},
                           {x0, y1, -halfThickness}}});
                }
                if (!itemAtlas.isOpaque(item, pixelX + 1, pixelY))
                {
                    edge({{{x1, y0, halfThickness},
                           {x1, y0, -halfThickness},
                           {x1, y1, -halfThickness},
                           {x1, y1, halfThickness}}});
                }
                if (!itemAtlas.isOpaque(item, pixelX, pixelY - 1))
                {
                    edge({{{x0, y1, halfThickness},
                           {x1, y1, halfThickness},
                           {x1, y1, -halfThickness},
                           {x0, y1, -halfThickness}}});
                }
                if (!itemAtlas.isOpaque(item, pixelX, pixelY + 1))
                {
                    edge({{{x0, y0, -halfThickness},
                           {x1, y0, -halfThickness},
                           {x1, y0, halfThickness},
                           {x0, y0, halfThickness}}});
                }
            }
        }
        return vertices;
    }

    const BlockType block = blockFromItem(item);
    const mc::content::ContentCatalog* catalog =
        mc::content::ContentCatalog::active();
    mc::content::BlockState itemState(block, 0);

    if (catalog != nullptr)
    {
        itemState = catalog->defaultState(block);

        const mc::content::BlockState metadataState(
            block, static_cast<std::uint16_t>(stack.damage)
        );
        if (catalog->isValidState(metadataState))
            itemState = metadataState;

        const mc::content::BlockDefinition* definition =
            catalog->block(itemState);

        if (const mc::core::ResourceLocation* itemName =
                catalog->itemName(item))
        {
            try
            {
                const mc::content::resources::ResourcePack resources(
                    AssetPaths::root()
                );
                const mc::client::ModelBakery bakery(resources, *catalog);
                const mc::core::ResourceLocation modelName(
                    itemName->nameSpace(),
                    std::string("item/") + itemName->path()
                );
                if (appendBakedModel(
                        vertices, bakery.bakeModel(modelName), definition))
                    return vertices;
            }
            catch (const std::exception&)
            {
                // builtin/entity and deliberately model-less ItemBlocks use
                // the block-model / legacy fallbacks below.
            }
        }

        if (const mc::client::BakedModel* baked =
                TextureAtlas::getBakedBlockModel(itemState, 0U))
        {
            if (appendBakedModel(vertices, *baked, definition))
                return vertices;
        }
    }

    const BlockShapeDefinition& shape = getBlockShape(itemState);

    if (shape.renderShape == BlockRenderShape::Boxes)
    {
        vertices.reserve(shape.renderBoxes.size() * 36U);
        for (const BlockBox& box : shape.renderBoxes)
        {
            for (const ItemFace& face : cubeFaces(box))
            {
                appendQuad(
                    vertices,
                    face.positions,
                    TextureAtlas::getBlockUV(itemState, face.blockFace),
                    face.shade,
                    FaceIndices
                );
            }
        }
        return vertices;
    }

    if (shape.renderShape == BlockRenderShape::CrossedPlanes)
    {
        constexpr float radius = 0.45f;
        const AtlasUV uv = TextureAtlas::getBlockUV(itemState, BlockFace::Front);
        appendQuad(
            vertices,
            {{{-radius, -0.5f, -radius}, {radius, -0.5f, radius},
              {radius, 0.5f, radius}, {-radius, 0.5f, -radius}}},
            uv,
            {1.0f, 1.0f, 1.0f},
            DoubleSidedIndices
        );
        appendQuad(
            vertices,
            {{{-radius, -0.5f, radius}, {radius, -0.5f, -radius},
              {radius, 0.5f, -radius}, {-radius, 0.5f, radius}}},
            uv,
            {1.0f, 1.0f, 1.0f},
            DoubleSidedIndices
        );
    }
    else if (shape.renderShape == BlockRenderShape::WallPlane)
    {
        appendQuad(
            vertices,
            {{{-0.5f, -0.5f, 0.0f}, {0.5f, -0.5f, 0.0f},
              {0.5f, 0.5f, 0.0f}, {-0.5f, 0.5f, 0.0f}}},
            TextureAtlas::getBlockUV(itemState, BlockFace::Front),
            {1.0f, 1.0f, 1.0f},
            DoubleSidedIndices
        );
    }
    return vertices;
}

void ItemEntityRenderer::upload(
    GpuMesh& mesh,
    const std::vector<Vertex>& vertices)
{
    mesh.vertexCount = static_cast<GLsizei>(vertices.size());
    if (vertices.empty())
        return;

    glGenVertexArrays(1, &mesh.vertexArray);
    glGenBuffers(1, &mesh.vertexBuffer);
    glBindVertexArray(mesh.vertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vertexBuffer);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)),
        vertices.data(),
        GL_STATIC_DRAW
    );
    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        reinterpret_cast<void*>(offsetof(Vertex, position))
    );
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        reinterpret_cast<void*>(offsetof(Vertex, uv))
    );
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        reinterpret_cast<void*>(offsetof(Vertex, colour))
    );
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
