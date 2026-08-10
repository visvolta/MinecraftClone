#include "Atmosphere.h"

#include "Player.h"
#include "World.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace
{
constexpr float PI = std::numbers::pi_v<float>;

float classicBrightness(int level)
{
    level = std::clamp(level, 0, 15);
    constexpr float ambientFloor = 0.05f;
    const float darkness = 1.0f - static_cast<float>(level) / 15.0f;
    return (1.0f - darkness) / (darkness * 3.0f + 1.0f) *
           (1.0f - ambientFloor) + ambientFloor;
}

float betaDayFactor(float celestialAngle)
{
    return std::clamp(
        std::cos(celestialAngle * PI * 2.0f) * 2.0f + 0.5f,
        0.0f,
        1.0f
    );
}

glm::vec3 hsbToRgb(float hue, float saturation, float brightness)
{
    if (saturation == 0.0f)
        return glm::vec3(brightness);

    const float wrappedHue = hue - std::floor(hue);
    const float scaledHue = wrappedHue * 6.0f;
    const int sector = static_cast<int>(std::floor(scaledHue));
    const float fraction = scaledHue - static_cast<float>(sector);
    const float p = brightness * (1.0f - saturation);
    const float q = brightness * (1.0f - saturation * fraction);
    const float t = brightness *
        (1.0f - saturation * (1.0f - fraction));

    switch (sector % 6)
    {
        case 0: return {brightness, t, p};
        case 1: return {q, brightness, p};
        case 2: return {p, brightness, t};
        case 3: return {p, q, brightness};
        case 4: return {t, p, brightness};
        default: return {brightness, p, q};
    }
}

float renderDistanceOption(float farPlaneDistance)
{
    return std::clamp(
        std::log2(256.0f / farPlaneDistance),
        0.0f,
        3.0f
    );
}
}

void Atmosphere::tick(
    const World& world,
    const glm::vec3& cameraPosition)
{
    ++worldTime_;
    previousFogExposure_ = fogExposure_;

    const int subtraction = skylightSubtracted(0.0f);
    const int x = static_cast<int>(std::floor(cameraPosition.x));
    const int y = static_cast<int>(std::floor(cameraPosition.y));
    const int z = static_cast<int>(std::floor(cameraPosition.z));
    const int skyLight = std::max(
        0,
        world.getSkyLightLevel(x, y, z) - subtraction
    );
    const int blockLight = world.getBlockLightLevel(x, y, z);
    const float localBrightness = classicBrightness(
        std::max(skyLight, blockLight)
    );

    // EntityRenderer biases fog exposure toward full brightness at farther
    // render-distance settings and eases toward the target by 10% per tick.
    const float farPlane = std::clamp(
        static_cast<float>(world.getRenderDistance()) * 16.0f,
        32.0f,
        256.0f
    );
    const float option = renderDistanceOption(farPlane);
    const float distanceBias = (3.0f - option) / 3.0f;
    const float target =
        localBrightness * (1.0f - distanceBias) + distanceBias;
    fogExposure_ += (target - fogExposure_) * 0.1f;
}

AtmosphereState Atmosphere::sample(
    const World& world,
    const Player& player,
    const glm::vec3& cameraPosition,
    int renderDistanceChunks,
    float partialTick) const
{
    partialTick = std::clamp(partialTick, 0.0f, 1.0f);

    AtmosphereState state;
    state.celestialAngle = celestialAngle(partialTick);
    const float dayFactor = betaDayFactor(state.celestialAngle);

    const int worldX = static_cast<int>(std::floor(cameraPosition.x));
    const int worldZ = static_cast<int>(std::floor(cameraPosition.z));
    float temperature = world.getTemperatureAt(worldX, worldZ) / 3.0f;
    temperature = std::clamp(temperature, -1.0f, 1.0f);
    state.skyColour = hsbToRgb(
        0.62222224f - temperature * 0.05f,
        0.5f + temperature * 0.1f,
        1.0f
    ) * dayFactor;

    glm::vec3 fogBase(0.7529412f, 0.84705883f, 1.0f);
    fogBase.r *= dayFactor * 0.94f + 0.06f;
    fogBase.g *= dayFactor * 0.94f + 0.06f;
    fogBase.b *= dayFactor * 0.91f + 0.09f;

    state.farPlaneDistance = std::clamp(
        static_cast<float>(renderDistanceChunks) * 16.0f,
        32.0f,
        256.0f
    );
    const float option = renderDistanceOption(state.farPlaneDistance);
    const float fogSkyBlend =
        1.0f - std::pow(1.0f / (4.0f - option), 0.25f);
    state.fogColour = fogBase +
        (state.skyColour - fogBase) * fogSkyBlend;

    state.fogMode = FogMode::Linear;
    state.fogStart = state.farPlaneDistance * 0.25f;
    state.fogEnd = state.farPlaneDistance;
    state.fogDensity = 0.0f;

    if (player.isHeadUnderwater())
    {
        state.fogColour = {0.02f, 0.02f, 0.2f};
        state.fogMode = FogMode::Exponential;
        state.fogDensity = 0.1f;
    }
    else if (player.isInLava())
    {
        state.fogColour = {0.6f, 0.1f, 0.0f};
        state.fogMode = FogMode::Exponential;
        state.fogDensity = 2.0f;
    }

    const float fogExposure = previousFogExposure_ +
        (fogExposure_ - previousFogExposure_) * partialTick;
    state.fogColour *= fogExposure;
    state.horizonColour = state.fogColour;
    state.lowerSkyColour = {
        state.skyColour.r * 0.2f + 0.04f,
        state.skyColour.g * 0.2f + 0.04f,
        state.skyColour.b * 0.6f + 0.1f
    };

    const float sunsetCosine =
        std::cos(state.celestialAngle * PI * 2.0f);
    if (sunsetCosine >= -0.4f && sunsetCosine <= 0.4f)
    {
        const float phase = sunsetCosine / 0.4f * 0.5f + 0.5f;
        float alpha =
            1.0f - (1.0f - std::sin(phase * PI)) * 0.99f;
        alpha *= alpha;
        state.sunriseSunsetColour = {
            phase * 0.3f + 0.7f,
            phase * phase * 0.7f + 0.2f,
            0.2f,
            alpha
        };
    }

    float star = 1.0f -
        (std::cos(state.celestialAngle * PI * 2.0f) * 2.0f + 0.75f);
    star = std::clamp(star, 0.0f, 1.0f);
    state.starBrightness = star * star * 0.5f;

    const int subtraction = skylightSubtracted(partialTick);
    state.daylightBrightness = classicBrightness(15 - subtraction);
    return state;
}

std::uint64_t Atmosphere::getWorldTime() const noexcept
{
    return worldTime_;
}

int Atmosphere::getDayTime() const noexcept
{
    return static_cast<int>(worldTime_ % DAY_LENGTH);
}

void Atmosphere::setDayTime(int ticks) noexcept
{
    const int wrapped = ((ticks % static_cast<int>(DAY_LENGTH)) +
        static_cast<int>(DAY_LENGTH)) % static_cast<int>(DAY_LENGTH);
    worldTime_ = worldTime_ / DAY_LENGTH * DAY_LENGTH +
        static_cast<std::uint64_t>(wrapped);
}

void Atmosphere::setWorldTime(std::uint64_t ticks) noexcept
{
    worldTime_ = ticks;
}

float Atmosphere::celestialAngle(float partialTick) const noexcept
{
    const int dayTime = static_cast<int>(worldTime_ % DAY_LENGTH);
    float angle =
        (static_cast<float>(dayTime) + partialTick) /
        static_cast<float>(DAY_LENGTH) - 0.25f;

    if (angle < 0.0f)
        angle += 1.0f;
    if (angle > 1.0f)
        angle -= 1.0f;

    const float original = angle;
    angle = 1.0f - (std::cos(angle * PI) + 1.0f) / 2.0f;
    return original + (angle - original) / 3.0f;
}

int Atmosphere::skylightSubtracted(float partialTick) const noexcept
{
    const float dayFactor = betaDayFactor(celestialAngle(partialTick));
    return static_cast<int>((1.0f - dayFactor) * 11.0f);
}
