#include "worldgen/BetaSimplexOctaves.h"

#include "worldgen/JavaRandom.h"

#include <cstddef>

BetaSimplexOctaves::BetaSimplexOctaves(
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

void BetaSimplexOctaves::generate(
    std::vector<double>& output,
    double originX,
    double originZ,
    int sizeX,
    int sizeZ,
    double scaleX,
    double scaleZ,
    double frequencyMultiplier,
    double amplitudeMultiplier) const
{
    scaleX /= 1.5;
    scaleZ /= 1.5;

    output.assign(
        static_cast<std::size_t>(sizeX * sizeZ),
        0.0
    );

    double amplitudeScale = 1.0;
    double frequencyScale = 1.0;

    for (const auto& octave : octaves_)
    {
        octave.add(
            output,
            originX,
            originZ,
            sizeX,
            sizeZ,
            scaleX * frequencyScale,
            scaleZ * frequencyScale,
            0.55 / amplitudeScale
        );

        frequencyScale *= frequencyMultiplier;
        amplitudeScale *= amplitudeMultiplier;
    }
}
