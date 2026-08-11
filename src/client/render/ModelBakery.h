#pragma once

#include "BlockShape.h"
#include "TextureAtlas.h"
#include "content/BlockState.h"
#include "content/resources/ResourcePack.h"

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace mc::content { class ContentCatalog; }

namespace mc::client
{
struct BakedQuad
{
    std::array<glm::vec3, 4> positions{};
    std::array<glm::vec2, 4> textureCoordinates{};
    core::ResourceLocation texture{"minecraft:blocks/debug"};
    BlockFace face = BlockFace::Front;
    std::optional<BlockFace> cullFace;
    int tintIndex = -1;
    bool shade = true;
};

struct BakedModel
{
    std::vector<BakedQuad> quads;
    // One axis-aligned box per resolved model element. The renderer keeps the
    // original quads, while collision/raycasting reuse these boxes so every
    // resource-defined state (stairs, slabs, doors, fences, panes, and modded
    // equivalents) has geometry matching the selected 1.12 model.
    std::vector<BlockBox> elementBoxes;
    std::unordered_map<
        std::string,
        content::resources::DisplayTransform
    > display;
    bool ambientOcclusion = true;
};

class ModelBakery
{
public:
    ModelBakery(
        const content::resources::ResourcePack& resources,
        const content::ContentCatalog& content
    );

    [[nodiscard]] BakedModel bake(
        content::BlockState state,
        std::uint64_t positionSeed = 0
    ) const;

private:
    const content::resources::ResourcePack& resources_;
    const content::ContentCatalog& content_;
};
}
