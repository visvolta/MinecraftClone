#pragma once
#include "Block.h"
#include "worldgen/Biome.h"
class JavaRandom;
class WorldGenerationContext;

class DecorationGenerator
{
public:
    void generateFlowers(WorldGenerationContext&, JavaRandom&, BlockType, int,int,int) const;
    void generateTallGrass(WorldGenerationContext&, JavaRandom&, int,int,int) const;
    void generatePumpkins(WorldGenerationContext&, JavaRandom&, int,int,int) const;
private:
    static bool canFlowerStay(
        const WorldGenerationContext&,
        int,
        int,
        int
    );
    static bool canMushroomStay(
        const WorldGenerationContext&,
        int,
        int,
        int
    );
    static int descendToGround(
        const WorldGenerationContext&,
        int,
        int,
        int
    );
};
