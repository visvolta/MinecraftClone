#include "Frustum.h"

#include <cmath>

Frustum::Frustum(const glm::mat4& m)
{
    // GLM matrices are column-major. Build planes from matrix rows.
    const glm::vec4 row0(m[0][0], m[1][0], m[2][0], m[3][0]);
    const glm::vec4 row1(m[0][1], m[1][1], m[2][1], m[3][1]);
    const glm::vec4 row2(m[0][2], m[1][2], m[2][2], m[3][2]);
    const glm::vec4 row3(m[0][3], m[1][3], m[2][3], m[3][3]);

    planes[0] = normalizePlane(row3 + row0); // left
    planes[1] = normalizePlane(row3 - row0); // right
    planes[2] = normalizePlane(row3 + row1); // bottom
    planes[3] = normalizePlane(row3 - row1); // top
    planes[4] = normalizePlane(row3 + row2); // near
    planes[5] = normalizePlane(row3 - row2); // far
}

bool Frustum::intersectsAabb(const glm::vec3& minimum,
                             const glm::vec3& maximum) const
{
    for (const glm::vec4& plane : planes)
    {
        const glm::vec3 positive(
            plane.x >= 0.0f ? maximum.x : minimum.x,
            plane.y >= 0.0f ? maximum.y : minimum.y,
            plane.z >= 0.0f ? maximum.z : minimum.z
        );
        if (glm::dot(glm::vec3(plane), positive) + plane.w < 0.0f)
            return false;
    }
    return true;
}

glm::vec4 Frustum::normalizePlane(const glm::vec4& plane)
{
    const float length = glm::length(glm::vec3(plane));
    return length > 0.0f ? plane / length : plane;
}
