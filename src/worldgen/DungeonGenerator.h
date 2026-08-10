#pragma once
class JavaRandom;
class WorldGenerationContext;
class DungeonGenerator
{
public:
    bool generate(WorldGenerationContext&, JavaRandom&, int,int,int) const;
};
