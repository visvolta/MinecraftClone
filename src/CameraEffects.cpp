#include "CameraEffects.h"

#include "Player.h"

#include <algorithm>
#include <cmath>

#include <glm/gtc/matrix_transform.hpp>

glm::mat4 CameraEffects::apply(
    const glm::mat4& view,
    const Player& player,
    float partialTick)
{
    partialTick = std::clamp(partialTick, 0.0f, 1.0f);
    glm::mat4 effect(1.0f);

    if (!player.isAlive())
    {
        const float deathTime =
            static_cast<float>(player.getDeathTicks()) + partialTick;
        const float deathRoll =
            40.0f - 8000.0f / (deathTime + 200.0f);
        effect = glm::rotate(
            effect,
            glm::radians(deathRoll),
            glm::vec3(0.0f, 0.0f, 1.0f)
        );
    }

    float hurtTime =
        static_cast<float>(player.getHurtCameraTicks()) - partialTick;
    if (hurtTime >= 0.0f)
    {
        hurtTime /= static_cast<float>(
            player.getMaximumHurtCameraTicks()
        );
        const float hurtStrength = std::sin(
            hurtTime * hurtTime * hurtTime * hurtTime * 3.1415927f
        );
        const float attackedYaw = player.getAttackedAtYaw();

        effect = glm::rotate(
            effect,
            glm::radians(-attackedYaw),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
        effect = glm::rotate(
            effect,
            glm::radians(-hurtStrength * 14.0f),
            glm::vec3(0.0f, 0.0f, 1.0f)
        );
        effect = glm::rotate(
            effect,
            glm::radians(attackedYaw),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
    }

    return effect * view;
}

float CameraEffects::getFieldOfView(
    float normalFieldOfView,
    const Player& player,
    float partialTick)
{
    partialTick = std::clamp(partialTick, 0.0f, 1.0f);
    float fieldOfView = normalFieldOfView *
        player.getFovMultiplier(partialTick);
    if (player.isHeadUnderwater())
        fieldOfView *= 60.0f / 70.0f;

    if (!player.isAlive())
    {
        const float deathTime =
            static_cast<float>(player.getDeathTicks()) + partialTick;
        fieldOfView /=
            (1.0f - 500.0f / (deathTime + 500.0f)) * 2.0f + 1.0f;
    }

    return fieldOfView;
}
