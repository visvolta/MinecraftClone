#pragma once

#include "AnimatedTexture.h"

#include <cstdint>

class Shader;

class FluidTextures
{
public:
    FluidTextures();

    void configureShader(const Shader& shader) const;
    void bindFrame(const Shader& shader, std::uint64_t gameTick) const;

private:
    static constexpr unsigned int WaterStillUnit = 1;
    static constexpr unsigned int WaterFlowUnit = 2;
    static constexpr unsigned int LavaStillUnit = 3;
    static constexpr unsigned int LavaFlowUnit = 4;

    AnimatedTexture waterStill_;
    AnimatedTexture waterFlow_;
    AnimatedTexture lavaStill_;
    AnimatedTexture lavaFlow_;
};
