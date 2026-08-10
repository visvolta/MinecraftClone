#pragma once

#include "Texture2D.h"
#include "TextureAtlas.h"
#include "core/ResourceLocation.h"

#include <cstddef>
#include <filesystem>
#include <memory>
#include <unordered_map>

namespace mc::client
{
class RuntimeTextureAtlas
{
public:
    static constexpr int TileSize = 16;
    static constexpr int Gutter = 1;
    static constexpr int CellSize = TileSize + Gutter * 2;

    explicit RuntimeTextureAtlas(const std::filesystem::path& blockTextureRoot);

    [[nodiscard]] const Texture2D& texture() const noexcept;
    [[nodiscard]] const AtlasUV* find(
        const core::ResourceLocation& textureName
    ) const noexcept;
    [[nodiscard]] const AtlasUV& missingTexture() const noexcept;
    [[nodiscard]] std::size_t textureCount() const noexcept;

private:
    std::unique_ptr<Texture2D> texture_;
    std::unordered_map<
        core::ResourceLocation,
        AtlasUV,
        core::ResourceLocationHash
    > entries_;
    AtlasUV missingTexture_{};
};
}
