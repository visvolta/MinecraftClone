#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

enum class MaterialTexture : std::uint8_t
{
    Atlas = 0,
    WaterStill = 1,
    WaterFlow = 2,
    LavaStill = 3,
    LavaFlow = 4
};

[[nodiscard]] constexpr float materialTextureAttribute(
    MaterialTexture texture) noexcept
{
    return static_cast<float>(texture);
}

struct ChunkVertex
{
    float positionX = 0.0f;
    float positionY = 0.0f;
    float positionZ = 0.0f;

    std::uint16_t textureU = 0;
    std::uint16_t textureV = 0;

    std::uint8_t tintRed = 255;
    std::uint8_t tintGreen = 255;
    std::uint8_t tintBlue = 255;

    // Grass-side overlay is sampled and composited in the same fragment as
    // the base grass-side texture. This completely removes z-fighting and
    // distant shimmering from two nearly coplanar faces.
    std::uint16_t overlayU = 0;
    std::uint16_t overlayV = 0;
    std::uint8_t overlayTintRed = 255;
    std::uint8_t overlayTintGreen = 255;
    std::uint8_t overlayTintBlue = 255;
    std::uint8_t hasOverlay = 0;

    // Used by the optional Beta fast-leaves shader path.
    std::uint8_t isLeaf = 0;
    std::uint8_t materialTexture =
        static_cast<std::uint8_t>(MaterialTexture::Atlas);

    ChunkVertex() noexcept = default;

    ChunkVertex(
        float x,
        float y,
        float z,
        float u,
        float v,
        float red,
        float green,
        float blue,
        float overlayTextureU,
        float overlayTextureV,
        float overlayRed,
        float overlayGreen,
        float overlayBlue,
        float overlayEnabled,
        float leaf,
        float material) noexcept
        : positionX(x),
          positionY(y),
          positionZ(z),
          textureU(packUnit(u)),
          textureV(packUnit(v)),
          tintRed(packColour(red)),
          tintGreen(packColour(green)),
          tintBlue(packColour(blue)),
          overlayU(packUnit(overlayTextureU)),
          overlayV(packUnit(overlayTextureV)),
          overlayTintRed(packColour(overlayRed)),
          overlayTintGreen(packColour(overlayGreen)),
          overlayTintBlue(packColour(overlayBlue)),
          hasOverlay(overlayEnabled > 0.5f ? 255U : 0U),
          isLeaf(leaf > 0.5f ? 255U : 0U),
          materialTexture(static_cast<std::uint8_t>(std::clamp(
              static_cast<int>(std::lround(material)), 0, 255
          )))
    {
    }

private:
    [[nodiscard]] static std::uint16_t packUnit(float value) noexcept
    {
        return static_cast<std::uint16_t>(std::lround(
            std::clamp(value, 0.0f, 1.0f) * 65535.0f
        ));
    }

    [[nodiscard]] static std::uint8_t packColour(float value) noexcept
    {
        return static_cast<std::uint8_t>(std::lround(
            std::clamp(value, 0.0f, 1.0f) * 255.0f
        ));
    }
};

static_assert(sizeof(ChunkVertex) == 32);
