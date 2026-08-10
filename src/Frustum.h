#pragma once

#include <array>
#include <glm/glm.hpp>

class Frustum
{
public:
    explicit Frustum(const glm::mat4& viewProjection);
    [[nodiscard]] bool intersectsAabb(const glm::vec3& minimum,
                                      const glm::vec3& maximum) const;

private:
    std::array<glm::vec4, 6> planes{};
    static glm::vec4 normalizePlane(const glm::vec4& plane);
};
