#include "TextureAtlas.h"

#include "BlockShape.h"
#include "client/render/RuntimeTextureAtlas.h"
#include "content/ContentCatalog.h"
#include "client/render/ModelBakery.h"

#include <array>
#include <exception>
#include <numeric>
#include <string_view>
#include <vector>

namespace
{
const mc::client::RuntimeTextureAtlas* RuntimeAtlas = nullptr;

struct CachedFace
{
    AtlasUV texture{};
    std::optional<AtlasUV> overlay;
};

std::vector<std::vector<std::array<CachedFace, 6>>> StateFaces;
std::vector<std::vector<std::vector<mc::client::BakedModel>>> StateModels;

constexpr std::size_t faceIndex(BlockFace face) noexcept
{
    return static_cast<std::size_t>(face);
}

bool endsWith(std::string_view value, std::string_view suffix) noexcept
{
    return value.size() >= suffix.size() &&
           value.substr(value.size() - suffix.size()) == suffix;
}

ModelBlockShapeKind modelShapeKind(
    std::string_view name,
    const mc::content::BlockDefinition& definition) noexcept
{
    if (name == "fence" || endsWith(name, "_fence"))
        return ModelBlockShapeKind::Fence;
    if (endsWith(name, "_wall"))
        return ModelBlockShapeKind::Wall;
    if (name == "glass_pane" || name == "iron_bars" ||
        endsWith(name, "_stained_glass_pane"))
        return ModelBlockShapeKind::Pane;
    if (!definition.behaviour.traits.solid)
        return ModelBlockShapeKind::NoCollision;
    return ModelBlockShapeKind::Solid;
}

}

void TextureAtlas::initialize(
    const mc::client::RuntimeTextureAtlas& atlas,
    const mc::content::ContentCatalog& content,
    const mc::content::resources::ResourcePack& resources)
{
    RuntimeAtlas = &atlas;

    clearModelBlockShapes();
    StateFaces.clear();
    StateFaces.resize(content.blocks().size());
    StateModels.clear();
    StateModels.resize(content.blocks().size());
    constexpr std::array<BlockFace, 6> faces{{
        BlockFace::Back, BlockFace::Front, BlockFace::Left,
        BlockFace::Right, BlockFace::Bottom, BlockFace::Top
    }};
    const mc::client::ModelBakery bakery(resources, content);
    for (const auto& entry : content.blocks().entries())
    {
        std::size_t variantCycle = 1U;
        try
        {
            const auto blockState = resources.loadBlockState(entry.name);
            for (const auto& [key, variants] : blockState.variants)
            {
                static_cast<void>(key);
                if (variants.size() > 1U)
                {
                    int totalWeight = 0;
                    for (const auto& variant : variants)
                        totalWeight += variant.weight;
                    variantCycle = std::lcm(
                        variantCycle,
                        static_cast<std::size_t>(totalWeight)
                    );
                }
            }
            for (const auto& part : blockState.multipart)
            {
                if (part.apply.size() > 1U)
                {
                    int totalWeight = 0;
                    for (const auto& variant : part.apply)
                        totalWeight += variant.weight;
                    variantCycle = std::lcm(
                        variantCycle,
                        static_cast<std::size_t>(totalWeight)
                    );
                }
            }
        }
        catch (const std::exception&)
        {
        }
        const std::size_t stateCount = entry.value.stateSchema.stateCount();
        StateFaces[entry.runtimeId].resize(stateCount);
        StateModels[entry.runtimeId].resize(stateCount);
        for (std::size_t properties = 0; properties < stateCount; ++properties)
        {
            const mc::content::BlockState state =
                mc::content::BlockState::fromRuntimeId(
                    entry.runtimeId, static_cast<std::uint16_t>(properties)
                );
            auto& cached = StateFaces[entry.runtimeId][properties];
            std::array<bool, 6> modelFaceFound{};
            if (entry.value.stateSchema.accepts(state))
            {
                try
                {
                    mc::client::BakedModel model = bakery.bake(state, 0U);
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
                    if (!entry.value.behaviour.traits.plant &&
                        !model.elementBoxes.empty())
                    {
                        registerModelBlockShape(
                            state,
                            model.elementBoxes,
                            modelShapeKind(entry.name.path(), entry.value),
                            entry.value.behaviour.traits.opaque
                        );
                    }
                    auto& variants = StateModels[entry.runtimeId][properties];
                    variants.push_back(std::move(model));
                    if (variantCycle > 1U)
                    {
                        variants.reserve(variantCycle);
                        for (std::size_t seed = 1;
                             seed < variantCycle; ++seed)
                            variants.push_back(bakery.bake(state, seed));
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
    const std::size_t block = state.blockRuntimeId();
    const std::size_t properties = state.properties();
    if (block < StateFaces.size() && properties < StateFaces[block].size())
        return StateFaces[block][properties][faceIndex(face)].texture;
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
    const std::size_t block = state.blockRuntimeId();
    const std::size_t properties = state.properties();
    return block < StateFaces.size() && properties < StateFaces[block].size()
        ? StateFaces[block][properties][faceIndex(face)].overlay
        : std::nullopt;
}

std::optional<AtlasUV> TextureAtlas::getBlockOverlayUV(
    BlockType block,
    BlockFace face) noexcept
{
    return getBlockOverlayUV(mc::content::BlockState(block), face);
}

const mc::client::BakedModel* TextureAtlas::getBakedBlockModel(
    mc::content::BlockState state,
    std::uint64_t positionSeed) noexcept
{
    const std::size_t block = state.blockRuntimeId();
    const std::size_t properties = state.properties();
    if (block >= StateModels.size() || properties >= StateModels[block].size())
        return nullptr;
    const auto& variants = StateModels[block][properties];
    if (variants.empty())
        return nullptr;
    const mc::client::BakedModel& model = variants[
        static_cast<std::size_t>(positionSeed % variants.size())
    ];
    return model.quads.empty() ? nullptr : &model;
}

const AtlasUV* TextureAtlas::getTextureUV(
    const mc::core::ResourceLocation& texture) noexcept
{
    return RuntimeAtlas == nullptr ? nullptr : RuntimeAtlas->find(texture);
}
