#include "Raycast.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "BlockShape.h"
#include "World.h"

namespace
{
    int stepFor(float value)
    {
        return value > 0.0f ? 1 : (value < 0.0f ? -1 : 0);
    }

    float firstBoundaryDistance(float origin, int voxel, float direction, int step)
    {
        if (step == 0)
            return std::numeric_limits<float>::infinity();

        const float boundary = step > 0
            ? static_cast<float>(voxel + 1)
            : static_cast<float>(voxel);

        return (boundary - origin) / direction;
    }

    bool intersectBox(
        const glm::vec3& origin,
        const glm::vec3& direction,
        const BlockBox& box,
        float maxDistance,
        float& distance,
        glm::ivec3& faceNormal)
    {
        float nearDistance = 0.0f;
        float farDistance = maxDistance;
        glm::ivec3 nearNormal(0);

        for (int axis = 0; axis < 3; ++axis)
        {
            if (std::abs(direction[axis]) < 0.000001f)
            {
                if (origin[axis] < box.minimum[axis] ||
                    origin[axis] > box.maximum[axis])
                {
                    return false;
                }
                continue;
            }

            float axisNear =
                (box.minimum[axis] - origin[axis]) / direction[axis];
            float axisFar =
                (box.maximum[axis] - origin[axis]) / direction[axis];
            int normalSign = -1;
            if (axisNear > axisFar)
            {
                std::swap(axisNear, axisFar);
                normalSign = 1;
            }
            if (axisNear > nearDistance)
            {
                nearDistance = axisNear;
                nearNormal = glm::ivec3(0);
                nearNormal[axis] = normalSign;
            }
            farDistance = std::min(farDistance, axisFar);
            if (nearDistance > farDistance)
                return false;
        }

        distance = nearDistance;
        faceNormal = nearNormal;
        return nearDistance <= maxDistance && farDistance >= 0.0f;
    }
}

RaycastHit Raycast::cast(
    const World& world,
    const glm::vec3& origin,
    const glm::vec3& direction,
    float maxDistance
)
{
    RaycastHit result;

    if (maxDistance <= 0.0f || glm::dot(direction, direction) < 0.000001f)
        return result;

    const glm::vec3 rayDirection = glm::normalize(direction);
    glm::ivec3 voxel(glm::floor(origin));

    const glm::ivec3 step(
        stepFor(rayDirection.x),
        stepFor(rayDirection.y),
        stepFor(rayDirection.z)
    );

    const glm::vec3 deltaDistance(
        step.x == 0 ? std::numeric_limits<float>::infinity() : std::abs(1.0f / rayDirection.x),
        step.y == 0 ? std::numeric_limits<float>::infinity() : std::abs(1.0f / rayDirection.y),
        step.z == 0 ? std::numeric_limits<float>::infinity() : std::abs(1.0f / rayDirection.z)
    );

    glm::vec3 sideDistance(
        firstBoundaryDistance(origin.x, voxel.x, rayDirection.x, step.x),
        firstBoundaryDistance(origin.y, voxel.y, rayDirection.y, step.y),
        firstBoundaryDistance(origin.z, voxel.z, rayDirection.z, step.z)
    );

    const auto testVoxel = [&world, &origin, &rayDirection, maxDistance](
        const glm::ivec3& position,
        RaycastHit& hit)
    {
        const mc::content::BlockState state =
            world.getActualBlockState(position.x, position.y, position.z);
        float closestDistance = maxDistance + 1.0f;
        glm::ivec3 closestNormal(0);
        bool found = false;
        for (const BlockBox& localBox : getBlockShape(state).selectionBoxes)
        {
            float distance = 0.0f;
            glm::ivec3 normal(0);
            const BlockBox worldBox = translateBlockBox(localBox, position);
            if (intersectBox(
                    origin,
                    rayDirection,
                    worldBox,
                    maxDistance,
                    distance,
                    normal) &&
                distance < closestDistance)
            {
                closestDistance = distance;
                closestNormal = normal;
                found = true;
            }
        }
        if (!found)
            return false;

        hit.hit = true;
        hit.blockPosition = position;
        hit.faceNormal = closestNormal;
        hit.previousPosition = position + closestNormal;
        hit.distance = closestDistance;
        return true;
    };

    if (testVoxel(voxel, result))
        return result;

    float travelled = 0.0f;

    while (travelled <= maxDistance)
    {
        if (sideDistance.x <= sideDistance.y && sideDistance.x <= sideDistance.z)
        {
            voxel.x += step.x;
            travelled = sideDistance.x;
            sideDistance.x += deltaDistance.x;
        }
        else if (sideDistance.y <= sideDistance.z)
        {
            voxel.y += step.y;
            travelled = sideDistance.y;
            sideDistance.y += deltaDistance.y;
        }
        else
        {
            voxel.z += step.z;
            travelled = sideDistance.z;
            sideDistance.z += deltaDistance.z;
        }

        if (travelled > maxDistance)
            break;

        if (testVoxel(voxel, result))
            return result;
    }

    return result;
}
