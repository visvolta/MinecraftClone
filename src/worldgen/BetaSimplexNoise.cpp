#include "worldgen/BetaSimplexNoise.h"

#include "worldgen/JavaRandom.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <utility>

namespace
{
constexpr std::array<std::array<int, 3>, 12> gradients{{
    {{1, 1, 0}},
    {{-1, 1, 0}},
    {{1, -1, 0}},
    {{-1, -1, 0}},
    {{1, 0, 1}},
    {{-1, 0, 1}},
    {{1, 0, -1}},
    {{-1, 0, -1}},
    {{0, 1, 1}},
    {{0, -1, 1}},
    {{0, 1, -1}},
    {{0, -1, -1}}
}};

const double skewFactor =
    0.5 * (std::sqrt(3.0) - 1.0);
const double unskewFactor =
    (3.0 - std::sqrt(3.0)) / 6.0;
}

BetaSimplexNoise::BetaSimplexNoise(JavaRandom& random)
    : offsetX_(random.nextDouble() * 256.0),
      offsetZ_(random.nextDouble() * 256.0),
      offsetY_(random.nextDouble() * 256.0)
{
    std::array<int, 256> source{};

    for (int index = 0; index < 256; ++index)
    {
        source[index] = index;
    }

    for (int index = 0; index < 256; ++index)
    {
        const int swapIndex =
            index + random.nextInt(256 - index);

        std::swap(
            source[index],
            source[swapIndex]
        );

        permutations_[index] = source[index];
        permutations_[index + 256] = source[index];
    }
}

void BetaSimplexNoise::add(
    std::vector<double>& output,
    double originX,
    double originZ,
    int sizeX,
    int sizeZ,
    double scaleX,
    double scaleZ,
    double amplitude) const
{
    std::size_t outputIndex = 0;

    for (int xIndex = 0;
         xIndex < sizeX;
         ++xIndex)
    {
        const double inputX =
            (originX + static_cast<double>(xIndex)) *
                scaleX +
            offsetX_;

        for (int zIndex = 0;
             zIndex < sizeZ;
             ++zIndex)
        {
            const double inputZ =
                (originZ + static_cast<double>(zIndex)) *
                    scaleZ +
                offsetZ_;

            const double skew =
                (inputX + inputZ) * skewFactor;

            const int cellX =
                fastFloor(inputX + skew);
            const int cellZ =
                fastFloor(inputZ + skew);

            const double unskew =
                static_cast<double>(cellX + cellZ) *
                unskewFactor;

            const double cellOriginX =
                static_cast<double>(cellX) - unskew;
            const double cellOriginZ =
                static_cast<double>(cellZ) - unskew;

            const double x0 = inputX - cellOriginX;
            const double z0 = inputZ - cellOriginZ;

            const int stepX = x0 > z0 ? 1 : 0;
            const int stepZ = x0 > z0 ? 0 : 1;

            const double x1 =
                x0 -
                static_cast<double>(stepX) +
                unskewFactor;
            const double z1 =
                z0 -
                static_cast<double>(stepZ) +
                unskewFactor;
            const double x2 =
                x0 - 1.0 + 2.0 * unskewFactor;
            const double z2 =
                z0 - 1.0 + 2.0 * unskewFactor;

            const int wrappedX = cellX & 255;
            const int wrappedZ = cellZ & 255;

            const int gradient0 =
                permutations_[
                    wrappedX +
                    permutations_[wrappedZ]
                ] %
                12;

            const int gradient1 =
                permutations_[
                    wrappedX +
                    stepX +
                    permutations_[wrappedZ + stepZ]
                ] %
                12;

            const int gradient2 =
                permutations_[
                    wrappedX +
                    1 +
                    permutations_[wrappedZ + 1]
                ] %
                12;

            double contribution0 = 0.0;
            double contribution1 = 0.0;
            double contribution2 = 0.0;

            double attenuation =
                0.5 - x0 * x0 - z0 * z0;

            if (attenuation >= 0.0)
            {
                attenuation *= attenuation;
                contribution0 =
                    attenuation *
                    attenuation *
                    gradientDot(gradient0, x0, z0);
            }

            attenuation =
                0.5 - x1 * x1 - z1 * z1;

            if (attenuation >= 0.0)
            {
                attenuation *= attenuation;
                contribution1 =
                    attenuation *
                    attenuation *
                    gradientDot(gradient1, x1, z1);
            }

            attenuation =
                0.5 - x2 * x2 - z2 * z2;

            if (attenuation >= 0.0)
            {
                attenuation *= attenuation;
                contribution2 =
                    attenuation *
                    attenuation *
                    gradientDot(gradient2, x2, z2);
            }

            output[outputIndex++] +=
                70.0 *
                (contribution0 +
                 contribution1 +
                 contribution2) *
                amplitude;
        }
    }
}

int BetaSimplexNoise::fastFloor(
    double value) noexcept
{
    // Reproduce Beta's NoiseGenerator2.wrap behaviour.
    return value > 0.0
        ? static_cast<int>(value)
        : static_cast<int>(value) - 1;
}

double BetaSimplexNoise::gradientDot(
    int gradientIndex,
    double x,
    double z) noexcept
{
    return
        static_cast<double>(
            gradients[
                static_cast<std::size_t>(gradientIndex)
            ][0]
        ) *
            x +
        static_cast<double>(
            gradients[
                static_cast<std::size_t>(gradientIndex)
            ][1]
        ) *
            z;
}
