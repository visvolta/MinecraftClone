#ifndef BETA_NOISE_GENERATOR_PERLIN_H
#define BETA_NOISE_GENERATOR_PERLIN_H

#include <array>
#include <vector>

class JavaRandom;

// Historical filename retained for compatibility. The implementation matches
// Minecraft 1.12.2 NoiseGeneratorImproved, including its ySize==1 fast path.
class BetaNoiseGeneratorPerlin
{
public:
    explicit BetaNoiseGeneratorPerlin(JavaRandom& random);

    [[nodiscard]] double noise(double x, double y) const;
    [[nodiscard]] double noise(double x, double y, double z) const;

    void populateNoiseArray(
        std::vector<double>& output,
        double xOffset,
        double yOffset,
        double zOffset,
        int xSize,
        int ySize,
        int zSize,
        double xScale,
        double yScale,
        double zScale,
        double noiseScale) const;

private:
    std::array<int, 512> permutations{};
    double offsetX = 0.0;
    double offsetY = 0.0;
    double offsetZ = 0.0;

    [[nodiscard]] static double fade(double value) noexcept;
    [[nodiscard]] static double lerp(double amount, double first, double second) noexcept;
    [[nodiscard]] static double grad(int hash, double x, double y, double z) noexcept;
    [[nodiscard]] static double grad2(int hash, double x, double z) noexcept;
};

#endif
