
#ifndef BETA_NOISE_GENERATOR_PERLIN_H
#define BETA_NOISE_GENERATOR_PERLIN_H

#include <array>

class JavaRandom;

class BetaNoiseGeneratorPerlin
{
public:
    explicit BetaNoiseGeneratorPerlin(JavaRandom& random);

    [[nodiscard]] double noise(double x, double y) const;
    [[nodiscard]] double noise(double x, double y, double z) const;

private:
    std::array<int, 512> permutations{};
    double offsetX = 0.0;
    double offsetY = 0.0;
    double offsetZ = 0.0;

    [[nodiscard]] static double fade(double value);
    [[nodiscard]] static double lerp(double amount, double first, double second);
    [[nodiscard]] static double gradient(int hash, double x, double y, double z);
};

#endif
