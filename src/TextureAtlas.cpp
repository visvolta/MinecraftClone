#include "TextureAtlas.h"

#include "client/render/RuntimeTextureAtlas.h"
#include "content/ContentCatalog.h"
#include "client/render/ModelBakery.h"

#include <array>
#include <exception>
#include <vector>

namespace
{
const mc::client::RuntimeTextureAtlas* RuntimeAtlas = nullptr;

struct CachedFace
{
    AtlasUV texture{};
    std::optional<AtlasUV> overlay;
};

std::vector<std::array<CachedFace, 6>> StateFaces;

constexpr std::size_t faceIndex(BlockFace face) noexcept
{
    return static_cast<std::size_t>(face);
}

std::size_t stateIndex(mc::content::BlockState state) noexcept
{
    return static_cast<std::size_t>(state.blockRuntimeId()) * 16U +
           state.properties();
}
}

void TextureAtlas::initialize(
    const mc::client::RuntimeTextureAtlas& atlas,
    const mc::content::ContentCatalog& content,
    const mc::content::resources::ResourcePack& resources)
{
    RuntimeAtlas = &atlas;

    StateFaces.clear();
    StateFaces.resize(content.blocks().size() * 16U);
    constexpr std::array<BlockFace, 6> faces{{
        BlockFace::Back, BlockFace::Front, BlockFace::Left,
        BlockFace::Right, BlockFace::Bottom, BlockFace::Top
    }};
    const mc::client::ModelBakery bakery(resources, content);
    for (const auto& entry : content.blocks().entries())
    {
        for (std::uint8_t properties = 0; properties < 16U; ++properties)
        {
            const mc::content::BlockState state =
                mc::content::BlockState::fromRuntimeId(
                    entry.runtimeId, properties
                );
            auto& cached = StateFaces[stateIndex(state)];
            std::array<bool, 6> modelFaceFound{};
            if (entry.value.stateSchema.accepts(state))
            {
                try
                {
                    const mc::client::BakedModel model = bakery.bake(state);
                    for (const mc::client::BakedQuad& quad : model.quads)
                    {
                        const std::size_t index = faceIndex(quad.face);
                        if (modelFaceFound[index])
                            continue;
                        if (const AtlasUV* uv = atlas.find(quad.texture))
                        {
                            cached[index].texture = *uv;
                            modelFaceFound[index] = true;
                        }
                    }
                }
                catch (const std::exception&)
                {
                    // Blocks rendered by a block entity (for example chests)
                    // intentionally have no ordinary baked model. Their
                    // registered compatibility texture remains the fallback.
                }
            }
            for (const BlockFace face : faces)
            {
                CachedFace& value = cached[faceIndex(face)];
                if (!modelFaceFound[faceIndex(face)])
                {
                    const mc::core::ResourceLocation* texture =
                        entry.value.textures.resolve(face, properties);
                    const AtlasUV* uv = texture == nullptr
                        ? nullptr
                        : atlas.find(*texture);
                    value.texture = uv == nullptr ? atlas.missingTexture() : *uv;
                }

                if (face != BlockFace::Top && face != BlockFace::Bottom &&
                    entry.value.textures.sideOverlay)
                {
                    if (const AtlasUV* overlay =
                            atlas.find(*entry.value.textures.sideOverlay))
                    {
                        value.overlay = *overlay;
                    }
                }
            }
        }
    }
}

AtlasUV TextureAtlas::getBlockUV(
    mc::content::BlockState state,
    BlockFace face) noexcept
{
    const std::size_t index = stateIndex(state);
    if (index < StateFaces.size())
        return StateFaces[index][faceIndex(face)].texture;
    return RuntimeAtlas == nullptr ? AtlasUV{} : RuntimeAtlas->missingTexture();
}

AtlasUV TextureAtlas::getBlockUV(
    BlockType block,
    BlockFace face,
    std::uint8_t metadata) noexcept
{
    if (RuntimeAtlas == nullptr)
        return {};
    const mc::content::BlockState state(block, metadata);
    return getBlockUV(state, face);
}

std::optional<AtlasUV> TextureAtlas::getBlockOverlayUV(
    mc::content::BlockState state,
    BlockFace face) noexcept
{
    if (face == BlockFace::Top || face == BlockFace::Bottom)
        return std::nullopt;
    const std::size_t index = stateIndex(state);
    return index < StateFaces.size()
        ? StateFaces[index][faceIndex(face)].overlay
        : std::nullopt;
}

std::optional<AtlasUV> TextureAtlas::getBlockOverlayUV(
    BlockType block,
    BlockFace face) noexcept
{
    return getBlockOverlayUV(mc::content::BlockState(block), face);
}
