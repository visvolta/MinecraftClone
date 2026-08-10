#pragma once

#include "Item.h"
#include "Texture2D.h"
#include "TextureAtlas.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <unordered_map>

class ItemAtlas
{
public:
    static constexpr int TILE_SIZE_PIXELS = 16;
    static constexpr int GUTTER_PIXELS = 1;
    static constexpr int CELL_SIZE_PIXELS = 18;

    // textureRoot is assets/textures. Standalone item sprites are stitched
    // from their individual 1.12 resource files at runtime, matching blocks.
    explicit ItemAtlas(const std::filesystem::path& textureRoot);

    [[nodiscard]] const Texture2D& texture() const noexcept;
    [[nodiscard]] AtlasUV getItemUV(ItemType item) const;
    [[nodiscard]] bool isOpaque(
        ItemType item,
        int pixelX,
        int pixelY
    ) const noexcept;

private:
    std::unique_ptr<Texture2D> texture_;
    std::unordered_map<ItemType, AtlasUV> entries_;
    std::unordered_map<ItemType, std::array<std::uint8_t, 256>> alpha_;
};
