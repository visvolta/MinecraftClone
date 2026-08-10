#pragma once

class JavaRandom;
class WorldGenerationContext;

class ClayGenerator
{
public:
    explicit ClayGenerator(int depositSize = 32);

    bool generate(
        WorldGenerationContext& context,
        JavaRandom& random,
        int worldX,
        int worldY,
        int worldZ) const;

private:
    int depositSize_ = 32;
};
