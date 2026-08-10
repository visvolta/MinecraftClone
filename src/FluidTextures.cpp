#include "FluidTextures.h"

#include "AssetPaths.h"
#include "Shader.h"

namespace
{
// AnimatedTexture strips advance at 10 frames per second. The game still
// simulates at Beta's 20 ticks per second; only the visual frame cadence is
// slowed so neither liquid animation races ahead of its block updates.
constexpr std::uint64_t LiquidAnimationFrameTicks = 2;

int frameFor(std::uint64_t gameTick, const AnimatedTexture& texture)
{
    const std::uint64_t animationFrame =
        gameTick / LiquidAnimationFrameTicks;
    return static_cast<int>(
        animationFrame %
        static_cast<std::uint64_t>(texture.getFrameCount())
    );
}
}

FluidTextures::FluidTextures()
    : waterStill_(AssetPaths::get("textures/blocks/water_still.png"), 16, 16),
      waterFlow_(AssetPaths::get("textures/blocks/water_flow.png"), 32, 32),
      lavaStill_(AssetPaths::get("textures/blocks/lava_still.png"), 16, 16),
      lavaFlow_(AssetPaths::get("textures/blocks/lava_flow.png"), 32, 32)
{
}

void FluidTextures::configureShader(const Shader& shader) const
{
    shader.use();
    shader.setInt("waterStillTexture", WaterStillUnit);
    shader.setInt("waterFlowTexture", WaterFlowUnit);
    shader.setInt("lavaStillTexture", LavaStillUnit);
    shader.setInt("lavaFlowTexture", LavaFlowUnit);
}

void FluidTextures::bindFrame(
    const Shader& shader,
    std::uint64_t gameTick) const
{
    // The ownership and units are fixed here: no render-site code can bind a
    // water animation to a lava sampler (or vice versa).
    waterStill_.bind(WaterStillUnit);
    waterFlow_.bind(WaterFlowUnit);
    lavaStill_.bind(LavaStillUnit);
    lavaFlow_.bind(LavaFlowUnit);

    shader.setInt("waterFrame", frameFor(gameTick, waterStill_));
    shader.setInt("waterFlowFrame", frameFor(gameTick, waterFlow_));
    shader.setInt("lavaFrame", frameFor(gameTick, lavaStill_));
    shader.setInt("lavaFlowFrame", frameFor(gameTick, lavaFlow_));
}
