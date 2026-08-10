#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

#include <glm/glm.hpp>

#include "worldgen/Biome.h"

class BiomeColorMap
{
public:
    BiomeColorMap(
        const std::filesystem::path& grassPath,
        const std::filesystem::path& foliagePath
    );

    [[nodiscard]] glm::vec3 getGrassColor(
        float temperature,
        float humidity,
        BiomeId biome = VanillaBiomes::Plains
    ) const noexcept;

    [[nodiscard]] glm::vec3 getFoliageColor(
        float temperature,
        float humidity,
        BiomeId biome = VanillaBiomes::Plains
    ) const noexcept;

private:
    struct Image
    {
        static constexpr int ExpectedWidth = 256;
        static constexpr int ExpectedHeight = 256;

        int width = 0;
        int height = 0;
        std::vector<std::uint8_t> pixels;

        [[nodiscard]] glm::vec3 sampleBetaColorizer(
            float temperature,
            float humidity
        ) const noexcept;
    };

    Image grassImage_;
    Image foliageImage_;

    [[nodiscard]] static Image loadColorMap(
        const std::filesystem::path& path
    );
};
