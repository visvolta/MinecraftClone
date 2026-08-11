#pragma once
#include "worldgen/BetaSimplexNoise.h"
#include <vector>
class JavaRandom;
class BetaSimplexOctaves
{
public:
    BetaSimplexOctaves(JavaRandom& random,int octaveCount);
    [[nodiscard]] double value(double x,double z) const;
    void generate(std::vector<double>& output,double originX,double originZ,
                  int sizeX,int sizeZ,double scaleX,double scaleZ,
                  double frequencyMultiplier,double amplitudeMultiplier=.5) const;
private: std::vector<BetaSimplexNoise> octaves_;
};
