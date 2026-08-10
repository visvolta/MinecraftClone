#pragma once

#include <cstdint>

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

class Player;
class World;

enum class FogMode
{
    Linear = 0,
    Exponential = 1
};

struct AtmosphereState
{
    glm::vec3 skyColour{0.0f};
    glm::vec3 horizonColour{0.0f};
    glm::vec3 lowerSkyColour{0.0f};
    glm::vec3 fogColour{0.0f};
    glm::vec4 sunriseSunsetColour{0.0f};

    float celestialAngle = 0.0f;
    float starBrightness = 0.0f;
    float daylightBrightness = 1.0f;
    float farPlaneDistance = 256.0f;
    float fogStart = 64.0f;
    float fogEnd = 256.0f;
    float fogDensity = 0.0f;
    FogMode fogMode = FogMode::Linear;
};

class Atmosphere
{
public:
    void tick(const World& world, const glm::vec3& cameraPosition);

    [[nodiscard]] AtmosphereState sample(
        const World& world,
        const Player& player,
        const glm::vec3& cameraPosition,
        int renderDistanceChunks,
        float partialTick
    ) const;

    [[nodiscard]] std::uint64_t getWorldTime() const noexcept;
    [[nodiscard]] int getDayTime() const noexcept;
    void setDayTime(int ticks) noexcept;
    void setWorldTime(std::uint64_t ticks) noexcept;

private:
    static constexpr std::uint64_t DAY_LENGTH = 24000;

    std::uint64_t worldTime_ = 0;
    float previousFogExposure_ = 1.0f;
    float fogExposure_ = 1.0f;

    [[nodiscard]] float celestialAngle(float partialTick) const noexcept;
    [[nodiscard]] int skylightSubtracted(float partialTick) const noexcept;
};
