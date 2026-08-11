#pragma once

#include "Block.h"
#include "content/BlockState.h"

#include <cstdint>
#include <optional>

enum class BlockFace { Back, Front, Left, Right, Bottom, Top };

struct AtlasUV
{
    float minU = 0.0f;
    float minV = 0.0f;
    float maxU = 1.0f;
    float maxV = 1.0f;
};

namespace mc::client { class RuntimeTextureAtlas; struct BakedModel; }
namespace mc::content { class ContentCatalog; }
namespace mc::content::resources { class ResourcePack; }

// Compatibility bridge for the existing mesher/UI. Texture identity now
// comes from the frozen content registry and UVs come from a runtime-stitched
// atlas. Stage 4 can replace BlockType+metadata calls with BlockState directly.
class TextureAtlas
{
public:
    static void initialize(
        const mc::client::RuntimeTextureAtlas& atlas,
        const mc::content::ContentCatalog& content,
        const mc::content::resources::ResourcePack& resources
    );

    [[nodiscard]] static AtlasUV getBlockUV(
        mc::content::BlockState state,
        BlockFace face
    ) noexcept;

    [[nodiscard]] static AtlasUV getBlockUV(
        BlockType block,
        BlockFace face,
        std::uint8_t metadata = 3
    ) noexcept;

    [[nodiscard]] static std::optional<AtlasUV> getBlockOverlayUV(
        mc::content::BlockState state,
        BlockFace face
    ) noexcept;

    [[nodiscard]] static std::optional<AtlasUV> getBlockOverlayUV(
        BlockType block,
        BlockFace face
    ) noexcept;
    [[nodiscard]] static const mc::client::BakedModel* getBakedBlockModel(
        mc::content::BlockState state,
        std::uint64_t positionSeed = 0
    ) noexcept;
    [[nodiscard]] static const AtlasUV* getTextureUV(
        const mc::core::ResourceLocation& texture
    ) noexcept;
};
