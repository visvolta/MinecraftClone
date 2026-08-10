#include "BiomeColorMap.h"

#include "worldgen/BetaSimplexNoise.h"
#include "worldgen/JavaRandom.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>

#include <stb_image.h>

namespace
{
[[nodiscard]] float byteToFloat(std::uint8_t value) noexcept
{
    return static_cast<float>(value) / 255.0f;
}

glm::vec3 packedColor(std::uint32_t color) noexcept
{
    return {
        byteToFloat(static_cast<std::uint8_t>((color >> 16U) & 0xFFU)),
        byteToFloat(static_cast<std::uint8_t>((color >> 8U) & 0xFFU)),
        byteToFloat(static_cast<std::uint8_t>(color & 0xFFU))
    };
}

double swampGrassNoise(int worldX, int worldZ)
{
    static JavaRandom random(2345LL);
    static const BetaSimplexNoise noise(random);
    std::vector<double> value(1, 0.0);
    noise.add(
        value,
        static_cast<double>(worldX),
        static_cast<double>(worldZ),
        1,
        1,
        0.0225,
        0.0225,
        1.0
    );
    return value.empty() ? 0.0 : value.front();
}
}

BiomeColorMap::BiomeColorMap(
    const std::filesystem::path& grassPath,
    const std::filesystem::path& foliagePath)
    : grassImage_(loadColorMap(grassPath)),
      foliageImage_(loadColorMap(foliagePath))
{
}

glm::vec3 BiomeColorMap::getGrassColor(
    float temperature,
    float humidity,
    BiomeId biome,
    int worldX,
    int worldZ) const noexcept
{
    if (biome == VanillaBiomes::Swampland ||
        biome == VanillaBiomes::SwamplandMountains)
    {
        return packedColor(
            swampGrassNoise(worldX, worldZ) < -0.1
                ? 0x4C763CU
                : 0x6A7039U
        );
    }
    if (const BiomeDefinition* definition = BiomeRegistry::active().find(biome);
        definition != nullptr && definition->grassColor)
        return packedColor(*definition->grassColor);
    return grassImage_.sampleBetaColorizer(temperature, humidity);
}

glm::vec3 BiomeColorMap::getFoliageColor(
    float temperature,
    float humidity,
    BiomeId biome,
    int,
    int) const noexcept
{
    if (const BiomeDefinition* definition = BiomeRegistry::active().find(biome);
        definition != nullptr && definition->foliageColor)
        return packedColor(*definition->foliageColor);
    return foliageImage_.sampleBetaColorizer(temperature, humidity);
}

BiomeColorMap::Image BiomeColorMap::loadColorMap(
    const std::filesystem::path& path)
{
    // Beta colorizer images are addressed from the top-left. They must not be
    // vertically flipped like OpenGL atlas textures.
    stbi_set_flip_vertically_on_load(false);

    int width = 0;
    int height = 0;
    int channels = 0;

    unsigned char* rawPixels = stbi_load(
        path.string().c_str(),
        &width,
        &height,
        &channels,
        STBI_rgb
    );

    if (rawPixels == nullptr)
    {
        const char* reason = stbi_failure_reason();
        throw std::runtime_error(
            "Failed to load biome color map: " + path.string() +
            (reason != nullptr
                ? "\nstb_image reason: " + std::string(reason)
                : std::string{})
        );
    }

    Image image;
    image.width = width;
    image.height = height;
    image.pixels.assign(
        rawPixels,
        rawPixels + static_cast<std::size_t>(width * height * 3)
    );
    stbi_image_free(rawPixels);

    if (image.width != Image::ExpectedWidth ||
        image.height != Image::ExpectedHeight)
    {
        throw std::runtime_error(
            "Biome color map must be exactly 256x256: " + path.string()
        );
    }

    return image;
}

glm::vec3 BiomeColorMap::Image::sampleBetaColorizer(
    float temperature,
    float humidity) const noexcept
{
    temperature = std::clamp(temperature, 0.0f, 1.0f);
    humidity = std::clamp(humidity, 0.0f, 1.0f);

    // Minecraft Beta 1.7.3 ColorizerGrass/ColorizerFoliage:
    // humidity *= temperature
    // x = int((1 - temperature) * 255)
    // y = int((1 - humidity) * 255)
    const float adjustedHumidity = humidity * temperature;

    const int x = std::clamp(
        static_cast<int>((1.0f - temperature) * 255.0f),
        0,
        255
    );
    const int y = std::clamp(
        static_cast<int>((1.0f - adjustedHumidity) * 255.0f),
        0,
        255
    );

    const std::size_t index = static_cast<std::size_t>(
        (y * ExpectedWidth + x) * 3
    );

    glm::vec3 color(
        byteToFloat(pixels[index]),
        byteToFloat(pixels[index + 1]),
        byteToFloat(pixels[index + 2])
    );

    if (!std::isfinite(color.r) ||
        !std::isfinite(color.g) ||
        !std::isfinite(color.b))
    {
        return glm::vec3(1.0f);
    }

    return color;
}
