#pragma once

#include <glm/mat4x4.hpp>

class Player;

namespace CameraEffects
{
[[nodiscard]] glm::mat4 apply(
    const glm::mat4& view,
    const Player& player,
    float partialTick
);

[[nodiscard]] float getFieldOfView(
    float normalFieldOfView,
    const Player& player,
    float partialTick
);
}
