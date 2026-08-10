#include "worldgen/BetaNoiseGeneratorPerlin.h"

#include <algorithm>
#include <cmath>

#include "worldgen/JavaRandom.h"

BetaNoiseGeneratorPerlin::BetaNoiseGeneratorPerlin(JavaRandom& random)
    : offsetX(random.nextDouble() * 256.0),
      offsetY(random.nextDouble() * 256.0),
      offsetZ(random.nextDouble() * 256.0)
{
    std::array<int, 256> source{};
    for (int index = 0; index < 256; ++index)
        source[index] = index;

    for (int index = 0; index < 256; ++index)
    {
        const int swapIndex = index + random.nextInt(256 - index);
        std::swap(source[index], source[swapIndex]);
        permutations[index] = source[index];
        permutations[index + 256] = source[index];
    }
}

double BetaNoiseGeneratorPerlin::noise(double x, double y) const
{
    return noise(x, y, 0.0);
}

double BetaNoiseGeneratorPerlin::noise(double x, double y, double z) const
{
    x += offsetX;
    y += offsetY;
    z += offsetZ;

    const int floorX = static_cast<int>(std::floor(x));
    const int floorY = static_cast<int>(std::floor(y));
    const int floorZ = static_cast<int>(std::floor(z));

    const int cubeX = floorX & 255;
    const int cubeY = floorY & 255;
    const int cubeZ = floorZ & 255;

    x -= static_cast<double>(floorX);
    y -= static_cast<double>(floorY);
    z -= static_cast<double>(floorZ);

    const double u = fade(x);
    const double v = fade(y);
    const double w = fade(z);

    const int a = permutations[cubeX] + cubeY;
    const int aa = permutations[a] + cubeZ;
    const int ab = permutations[a + 1] + cubeZ;
    const int b = permutations[cubeX + 1] + cubeY;
    const int ba = permutations[b] + cubeZ;
    const int bb = permutations[b + 1] + cubeZ;

    return lerp(w,
        lerp(v,
            lerp(u, gradient(permutations[aa], x, y, z),
                    gradient(permutations[ba], x - 1.0, y, z)),
            lerp(u, gradient(permutations[ab], x, y - 1.0, z),
                    gradient(permutations[bb], x - 1.0, y - 1.0, z))),
        lerp(v,
            lerp(u, gradient(permutations[aa + 1], x, y, z - 1.0),
                    gradient(permutations[ba + 1], x - 1.0, y, z - 1.0)),
            lerp(u, gradient(permutations[ab + 1], x, y - 1.0, z - 1.0),
                    gradient(permutations[bb + 1], x - 1.0, y - 1.0, z - 1.0))));
}

double BetaNoiseGeneratorPerlin::fade(double value)
{
    return value * value * value * (value * (value * 6.0 - 15.0) + 10.0);
}

double BetaNoiseGeneratorPerlin::lerp(double amount, double first, double second)
{
    return first + amount * (second - first);
}

double BetaNoiseGeneratorPerlin::gradient(int hash, double x, double y, double z)
{
    const int gradientIndex = hash & 15;
    const double first = gradientIndex < 8 ? x : y;
    const double second = gradientIndex < 4 ? y : (gradientIndex == 12 || gradientIndex == 14 ? x : z);
    return ((gradientIndex & 1) == 0 ? first : -first) +
           ((gradientIndex & 2) == 0 ? second : -second);
}
