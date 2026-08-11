#pragma once

#include <array>
#include <vector>

class JavaRandom;

class BetaSimplexNoise
{
public:
    explicit BetaSimplexNoise(JavaRandom& random);
    [[nodiscard]] double value(double x, double z) const;
    void add(std::vector<double>& output,double originX,double originZ,
             int sizeX,int sizeZ,double scaleX,double scaleZ,double amplitude) const;
private:
    std::array<int,512> permutations_{};
    double offsetX_=0.0,offsetZ_=0.0,offsetY_=0.0;
    [[nodiscard]] static int fastFloor(double value) noexcept;
    [[nodiscard]] static double gradientDot(int gradientIndex,double x,double z) noexcept;
};
