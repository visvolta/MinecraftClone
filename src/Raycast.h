#pragma once

#include <glm/glm.hpp>

class World;

struct RaycastHit
{
    bool hit = false;
    glm::ivec3 blockPosition{0};
    glm::ivec3 previousPosition{0};
    glm::ivec3 faceNormal{0};
    float distance = 0.0f;
};

class Raycast
{
public:
    [[nodiscard]] static RaycastHit cast(
        const World& world,
        const glm::vec3& origin,
        const glm::vec3& direction,
        float maxDistance
    );
};
