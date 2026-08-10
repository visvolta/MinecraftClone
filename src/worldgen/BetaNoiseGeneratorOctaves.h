#pragma once

#include <vector>

#include "worldgen/BetaNoiseGeneratorPerlin.h"

class JavaRandom;

class BetaNoiseGeneratorOctaves
{
public:
    BetaNoiseGeneratorOctaves(
        JavaRandom& random,
        int octaveCount);

    [[nodiscard]] double noise2D(
        double x,
        double z,
        double scale) const;

    [[nodiscard]] double noise3D(
        double x,
        double y,
        double z,
        double scaleX,
        double scaleY,
        double scaleZ) const;

    void generateNoiseOctaves(
        std::vector<double>& output,
        double originX,
        double originY,
        double originZ,
        int sizeX,
        int sizeY,
        int sizeZ,
        double scaleX,
        double scaleY,
        double scaleZ) const;

    void generateNoise2D(
        std::vector<double>& output,
        int originX,
        int originZ,
        int sizeX,
        int sizeZ,
        double scaleX,
        double scaleZ) const;

private:
    std::vector<BetaNoiseGeneratorPerlin> octaves_;
};
