#include "client/render/RuntimeTextureAtlas.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <ranges>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <stb_image.h>

namespace mc::client
{
namespace
{
struct SourceTexture
{
    core::ResourceLocation name;
    std::filesystem::path path;
};

std::vector<SourceTexture> discover(const std::filesystem::path& root)
{
    if (!std::filesystem::is_directory(root))
        throw std::runtime_error("Block texture directory is missing: " + root.string());

    std::vector<SourceTexture> textures;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(root))
    {
        if (!entry.is_regular_file() || entry.path().extension() != ".png")
            continue;
        std::filesystem::path relative = std::filesystem::relative(entry.path(), root);
        relative.replace_extension();
        const std::string resourcePath =
            std::string("blocks/") + relative.generic_string();
        textures.push_back({
            core::ResourceLocation("minecraft", resourcePath),
            entry.path()
        });
    }
    std::ranges::sort(
        textures,
        [](const SourceTexture& left, const SourceTexture& right)
        {
            return left.name < right.name;
        }
    );
    if (textures.empty())
        throw std::runtime_error("Block texture directory contains no PNG files");
    return textures;
}

void setPixel(
    std::vector<std::uint8_t>& pixels,
    int width,
    int x,
    int y,
    const std::uint8_t* colour)
{
    const std::size_t destination = static_cast<std::size_t>(
        (y * width + x) * 4
    );
    std::copy_n(colour, 4, pixels.data() + destination);
}
}

RuntimeTextureAtlas::RuntimeTextureAtlas(
    const std::filesystem::path& blockTextureRoot)
{
    const std::vector<SourceTexture> sources = discover(blockTextureRoot);
    const int columns = static_cast<int>(std::ceil(
        std::sqrt(static_cast<double>(sources.size()))
    ));
    const int rows = static_cast<int>(
        (sources.size() + static_cast<std::size_t>(columns) - 1U) /
        static_cast<std::size_t>(columns)
    );
    const int atlasWidth = columns * CellSize;
    const int atlasHeight = rows * CellSize;
    std::vector<std::uint8_t> topDownPixels(
        static_cast<std::size_t>(atlasWidth * atlasHeight * 4), 0
    );

    entries_.reserve(sources.size());
    for (std::size_t index = 0; index < sources.size(); ++index)
    {
        int sourceWidth = 0;
        int sourceHeight = 0;
        int channels = 0;
        stbi_set_flip_vertically_on_load(false);
        unsigned char* sourcePixels = stbi_load(
            sources[index].path.string().c_str(),
            &sourceWidth,
            &sourceHeight,
            &channels,
            4
        );
        if (sourcePixels == nullptr ||
            sourceWidth < TileSize || sourceHeight < TileSize)
        {
            if (sourcePixels != nullptr)
                stbi_image_free(sourcePixels);
            throw std::runtime_error(
                "Block texture must contain at least one 16x16 frame: " +
                sources[index].path.string()
            );
        }

        const int cellX = static_cast<int>(index % static_cast<std::size_t>(columns)) * CellSize;
        const int cellY = static_cast<int>(index / static_cast<std::size_t>(columns)) * CellSize;
        for (int destinationY = 0; destinationY < CellSize; ++destinationY)
        {
            const int sourceY = std::clamp(destinationY - Gutter, 0, TileSize - 1);
            for (int destinationX = 0; destinationX < CellSize; ++destinationX)
            {
                const int sourceX = std::clamp(destinationX - Gutter, 0, TileSize - 1);
                const std::uint8_t* colour = sourcePixels +
                    static_cast<std::size_t>((sourceY * sourceWidth + sourceX) * 4);
                setPixel(
                    topDownPixels,
                    atlasWidth,
                    cellX + destinationX,
                    cellY + destinationY,
                    colour
                );
            }
        }
        stbi_image_free(sourcePixels);

        constexpr float inset = 0.01f;
        const int sourceX = cellX + Gutter;
        const int sourceY = cellY + Gutter;
        const int glBottom = atlasHeight - sourceY - TileSize;
        const int glTop = atlasHeight - sourceY;
        const AtlasUV uv{
            (static_cast<float>(sourceX) + inset) / static_cast<float>(atlasWidth),
            (static_cast<float>(glBottom) + inset) / static_cast<float>(atlasHeight),
            (static_cast<float>(sourceX + TileSize) - inset) / static_cast<float>(atlasWidth),
            (static_cast<float>(glTop) - inset) / static_cast<float>(atlasHeight)
        };
        entries_.emplace(sources[index].name, uv);
    }

    // Texture2D expects the first uploaded row to be the OpenGL bottom row.
    std::vector<std::uint8_t> bottomUpPixels(topDownPixels.size());
    const std::size_t rowBytes = static_cast<std::size_t>(atlasWidth * 4);
    for (int y = 0; y < atlasHeight; ++y)
    {
        std::copy_n(
            topDownPixels.data() + static_cast<std::size_t>(y) * rowBytes,
            rowBytes,
            bottomUpPixels.data() +
                static_cast<std::size_t>(atlasHeight - 1 - y) * rowBytes
        );
    }
    texture_ = std::make_unique<Texture2D>(
        atlasWidth, atlasHeight, bottomUpPixels
    );

    const core::ResourceLocation preferredMissing("minecraft:blocks/debug");
    if (const AtlasUV* missing = find(preferredMissing))
        missingTexture_ = *missing;
    else
        missingTexture_ = entries_.begin()->second;
}

const Texture2D& RuntimeTextureAtlas::texture() const noexcept
{
    return *texture_;
}

const AtlasUV* RuntimeTextureAtlas::find(
    const core::ResourceLocation& textureName) const noexcept
{
    const auto found = entries_.find(textureName);
    return found == entries_.end() ? nullptr : &found->second;
}

const AtlasUV& RuntimeTextureAtlas::missingTexture() const noexcept
{
    return missingTexture_;
}

std::size_t RuntimeTextureAtlas::textureCount() const noexcept
{
    return entries_.size();
}
}
