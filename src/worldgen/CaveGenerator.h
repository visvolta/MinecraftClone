#pragma once

#include <cstdint>

class Chunk;
class JavaRandom;

class CaveGenerator
{
public:
    explicit CaveGenerator(std::int64_t worldSeed, int range = 8);
    void generate(Chunk& targetChunk) const;

private:
    std::int64_t worldSeed_ = 0;
    int range_ = 8;

    void recursiveGenerate(JavaRandom& random, int sourceChunkX, int sourceChunkZ,
                           Chunk& targetChunk) const;
    void generateLargeCaveNode(JavaRandom& random, Chunk& targetChunk,
                               double worldX, double worldY, double worldZ) const;
    void generateCaveNode(JavaRandom& parentRandom, Chunk& targetChunk,
                          double worldX, double worldY, double worldZ,
                          float radius, float yaw, float pitch,
                          int step, int maxSteps, double verticalScale) const;
};
