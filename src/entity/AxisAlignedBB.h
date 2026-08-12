#pragma once

#include <algorithm>
#include <cmath>

namespace mc::entity
{
struct AxisAlignedBB
{
    double minX = 0.0;
    double minY = 0.0;
    double minZ = 0.0;
    double maxX = 0.0;
    double maxY = 0.0;
    double maxZ = 0.0;

    [[nodiscard]] AxisAlignedBB offset(double x, double y, double z) const noexcept
    {
        return {minX + x, minY + y, minZ + z, maxX + x, maxY + y, maxZ + z};
    }

    [[nodiscard]] AxisAlignedBB grow(double x, double y, double z) const noexcept
    {
        return {minX - x, minY - y, minZ - z, maxX + x, maxY + y, maxZ + z};
    }

    [[nodiscard]] AxisAlignedBB expand(double x, double y, double z) const noexcept
    {
        AxisAlignedBB result = *this;
        if (x < 0.0) result.minX += x;
        else result.maxX += x;
        if (y < 0.0) result.minY += y;
        else result.maxY += y;
        if (z < 0.0) result.minZ += z;
        else result.maxZ += z;
        return result;
    }

    [[nodiscard]] AxisAlignedBB contract(double value) const noexcept
    {
        return grow(-value, -value, -value);
    }

    [[nodiscard]] bool intersects(const AxisAlignedBB& other) const noexcept
    {
        return minX < other.maxX && maxX > other.minX &&
               minY < other.maxY && maxY > other.minY &&
               minZ < other.maxZ && maxZ > other.minZ;
    }

    [[nodiscard]] bool intersects(
        double x1, double y1, double z1,
        double x2, double y2, double z2) const noexcept
    {
        return minX < x2 && maxX > x1 &&
               minY < y2 && maxY > y1 &&
               minZ < z2 && maxZ > z1;
    }

    [[nodiscard]] double calculateXOffset(
        const AxisAlignedBB& other,
        double offset) const noexcept
    {
        if (other.maxY <= minY || other.minY >= maxY ||
            other.maxZ <= minZ || other.minZ >= maxZ)
            return offset;
        if (offset > 0.0 && other.maxX <= minX)
        {
            const double delta = minX - other.maxX;
            if (delta < offset)
                return delta;
        }
        else if (offset < 0.0 && other.minX >= maxX)
        {
            const double delta = maxX - other.minX;
            if (delta > offset)
                return delta;
        }
        return offset;
    }

    [[nodiscard]] double calculateYOffset(
        const AxisAlignedBB& other,
        double offset) const noexcept
    {
        if (other.maxX <= minX || other.minX >= maxX ||
            other.maxZ <= minZ || other.minZ >= maxZ)
            return offset;
        if (offset > 0.0 && other.maxY <= minY)
        {
            const double delta = minY - other.maxY;
            if (delta < offset)
                return delta;
        }
        else if (offset < 0.0 && other.minY >= maxY)
        {
            const double delta = maxY - other.minY;
            if (delta > offset)
                return delta;
        }
        return offset;
    }

    [[nodiscard]] double calculateZOffset(
        const AxisAlignedBB& other,
        double offset) const noexcept
    {
        if (other.maxX <= minX || other.minX >= maxX ||
            other.maxY <= minY || other.minY >= maxY)
            return offset;
        if (offset > 0.0 && other.maxZ <= minZ)
        {
            const double delta = minZ - other.maxZ;
            if (delta < offset)
                return delta;
        }
        else if (offset < 0.0 && other.minZ >= maxZ)
        {
            const double delta = maxZ - other.minZ;
            if (delta > offset)
                return delta;
        }
        return offset;
    }

    [[nodiscard]] double getAverageEdgeLength() const noexcept
    {
        return (maxX - minX + maxY - minY + maxZ - minZ) / 3.0;
    }

    [[nodiscard]] double getCenterX() const noexcept { return (minX + maxX) * 0.5; }
    [[nodiscard]] double getCenterY() const noexcept { return (minY + maxY) * 0.5; }
    [[nodiscard]] double getCenterZ() const noexcept { return (minZ + maxZ) * 0.5; }
};
}
