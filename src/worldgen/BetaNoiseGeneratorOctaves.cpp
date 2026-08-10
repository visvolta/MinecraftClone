#include "worldgen/BetaNoiseGeneratorOctaves.h"

#include "worldgen/JavaRandom.h"

#include <algorithm>
#include <cstddef>

BetaNoiseGeneratorOctaves::BetaNoiseGeneratorOctaves(
    JavaRandom& random,
    int octaveCount)
{
    octaves_.reserve(
        static_cast<std::size_t>(octaveCount)
    );

    for (int octave = 0;
         octave < octaveCount;
         ++octave)
    {
        octaves_.emplace_back(random);
    }
}

double BetaNoiseGeneratorOctaves::noise2D(
    double x,
    double z,
    double scale) const
{
    double result = 0.0;
    double octaveScale = 1.0;

    for (const auto& octave : octaves_)
    {
        result +=
            octave.noise(
                x * scale * octaveScale,
                z * scale * octaveScale
            ) /
            octaveScale;

        octaveScale /= 2.0;
    }

    return result;
}

double BetaNoiseGeneratorOctaves::noise3D(
    double x,
    double y,
    double z,
    double scaleX,
    double scaleY,
    double scaleZ) const
{
    double result = 0.0;
    double octaveScale = 1.0;

    for (const auto& octave : octaves_)
    {
        result +=
            octave.noise(
                x * scaleX * octaveScale,
                y * scaleY * octaveScale,
                z * scaleZ * octaveScale
            ) /
            octaveScale;

        octaveScale /= 2.0;
    }

    return result;
}

void BetaNoiseGeneratorOctaves::generateNoiseOctaves(
    std::vector<double>& output,
    double originX,
    double originY,
    double originZ,
    int sizeX,
    int sizeY,
    int sizeZ,
    double scaleX,
    double scaleY,
    double scaleZ) const
{
    const std::size_t requiredSize =
        static_cast<std::size_t>(
            sizeX * sizeY * sizeZ
        );

    output.assign(requiredSize, 0.0);

    double octaveScale = 1.0;

    for (const auto& octave : octaves_)
    {
        const double contributionScale =
            1.0 / octaveScale;

        std::size_t index = 0;

        for (int x = 0; x < sizeX; ++x)
        {
            const double sampleX =
                (originX + static_cast<double>(x)) *
                scaleX *
                octaveScale;

            for (int z = 0; z < sizeZ; ++z)
            {
                const double sampleZ =
                    (originZ + static_cast<double>(z)) *
                    scaleZ *
                    octaveScale;

                for (int y = 0; y < sizeY; ++y)
                {
                    const double sampleY =
                        (originY + static_cast<double>(y)) *
                        scaleY *
                        octaveScale;

                    output[index++] +=
                        octave.noise(
                            sampleX,
                            sampleY,
                            sampleZ
                        ) *
                        contributionScale;
                }
            }
        }

        octaveScale /= 2.0;
    }
}

void BetaNoiseGeneratorOctaves::generateNoise2D(
    std::vector<double>& output,
    int originX,
    int originZ,
    int sizeX,
    int sizeZ,
    double scaleX,
    double scaleZ) const
{
    generateNoiseOctaves(
        output,
        static_cast<double>(originX),
        10.0,
        static_cast<double>(originZ),
        sizeX,
        1,
        sizeZ,
        scaleX,
        1.0,
        scaleZ
    );
}
