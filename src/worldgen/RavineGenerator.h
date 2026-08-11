#pragma once

#include <cstdint>

class Chunk;
class JavaRandom;

class RavineGenerator
{
public:
    explicit RavineGenerator(std::int64_t worldSeed, int range = 8);
    void generate(Chunk& targetChunk) const;

private:
    std::int64_t worldSeed_ = 0;
    int range_ = 8;

    void generateFrom(
        JavaRandom& random,
        int sourceChunkX,
        int sourceChunkZ,
        Chunk& targetChunk) const;
};
